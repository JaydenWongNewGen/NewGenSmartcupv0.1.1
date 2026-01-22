#include "QMI8658.h"
#include <Arduino.h>

#define QMI8658_SLAVE_ADDR_L 0x6b
#define QMI8658_SLAVE_ADDR_H 0x6b

static TwoWire* qmiWire = &Wire;
static unsigned char QMI8658_slave_addr = QMI8658_SLAVE_ADDR_L;

static unsigned short acc_lsb_div = 0;
static unsigned short gyro_lsb_div = 0;
static unsigned short ae_q_lsb_div = (1 << 14);
static unsigned short ae_v_lsb_div = (1 << 10);
static unsigned int imu_timestamp = 0;
static struct QMI8658Config QMI8658_config;

unsigned char QMI8658_write_reg(unsigned char reg, unsigned char value) {
    qmiWire->beginTransmission(QMI8658_slave_addr);
    qmiWire->write(reg);
    qmiWire->write(value);
    return (qmiWire->endTransmission() == 0) ? 1 : 0;
}

unsigned char QMI8658_write_regs(unsigned char reg, unsigned char *value, unsigned char len) {
    qmiWire->beginTransmission(QMI8658_slave_addr);
    qmiWire->write(reg);
    for (int i = 0; i < len; i++) {
        qmiWire->write(value[i]);
    }
    return (qmiWire->endTransmission() == 0) ? 1 : 0;
}

unsigned char QMI8658_read_reg(unsigned char reg, unsigned char *buf, unsigned short len) {
    qmiWire->beginTransmission(QMI8658_slave_addr);
    qmiWire->write(reg);
    if (qmiWire->endTransmission(false) != 0) return 0;
    if (qmiWire->requestFrom(QMI8658_slave_addr, len) != len) return 0;
    for (int i = 0; i < len && qmiWire->available(); i++) {
        buf[i] = qmiWire->read();
    }
    return 1;
}

// Custom init for any TwoWire instance
unsigned char QMI8658_init(TwoWire& wire) {
    qmiWire = &wire;
    return QMI8658_init();
}

unsigned char QMI8658_init(void) {
    unsigned char QMI8658_chip_id = 0x00;
    unsigned char QMI8658_revision_id = 0x00;
    unsigned char QMI8658_slave[2] = { QMI8658_SLAVE_ADDR_L, QMI8658_SLAVE_ADDR_H };

    for (int i = 0; i < 2; i++) {
        QMI8658_slave_addr = QMI8658_slave[i];

        for (int retry = 0; retry < 5; retry++) {
            QMI8658_read_reg(QMI8658Register_WhoAmI, &QMI8658_chip_id, 1);
            Serial.print("QMI8658Register_WhoAmI = ");
            Serial.println(QMI8658_chip_id);
            if (QMI8658_chip_id == 0x05) break;
        }

        if (QMI8658_chip_id == 0x05) break;
    }

    if (QMI8658_chip_id != 0x05) {
        Serial.println("QMI8658_init fail");
        return 0;
    }

    QMI8658_read_reg(QMI8658Register_Revision, &QMI8658_revision_id, 1);

    Serial.print("QMI8658_init slave = ");
    Serial.println(QMI8658_slave_addr);
    Serial.print("QMI8658Register_WhoAmI = ");
    Serial.print(QMI8658_chip_id);
    Serial.print(" Revision = ");
    Serial.println(QMI8658_revision_id);

    QMI8658_write_reg(QMI8658Register_Ctrl1, 0x60);

    QMI8658_config.inputSelection = QMI8658_CONFIG_ACCGYR_ENABLE;
    QMI8658_config.accRange = QMI8658AccRange_8g;
    QMI8658_config.accOdr = QMI8658AccOdr_1000Hz;
    QMI8658_config.gyrRange = QMI8658GyrRange_512dps;
    QMI8658_config.gyrOdr = QMI8658GyrOdr_1000Hz;
    QMI8658_config.magOdr = QMI8658MagOdr_125Hz;
    QMI8658_config.magDev = MagDev_AKM09918;
    QMI8658_config.aeOdr = QMI8658AeOdr_128Hz;

    QMI8658_Config_apply(&QMI8658_config);
    return 1;
}

void QMI8658_config_acc(QMI8658_AccRange range, QMI8658_AccOdr odr, QMI8658_LpfConfig lpf, QMI8658_StConfig st) {
    unsigned char data;

    switch (range) {
        case QMI8658AccRange_2g:  acc_lsb_div = (1 << 14); break;
        case QMI8658AccRange_4g:  acc_lsb_div = (1 << 13); break;
        case QMI8658AccRange_8g:  acc_lsb_div = (1 << 12); break;
        case QMI8658AccRange_16g: acc_lsb_div = (1 << 11); break;
        default: acc_lsb_div = (1 << 12); break;
    }

    data = range | odr;
    if (st == QMI8658St_Enable) data |= 0x80;
    QMI8658_write_reg(QMI8658Register_Ctrl2, data);

    QMI8658_read_reg(QMI8658Register_Ctrl5, &data, 1);
    data &= 0xf0;
    data |= (lpf == QMI8658Lpf_Enable) ? (A_LSP_MODE_3 | 0x01) : 0;
    QMI8658_write_reg(QMI8658Register_Ctrl5, data);
}

void QMI8658_config_gyro(QMI8658_GyrRange range, QMI8658_GyrOdr odr, QMI8658_LpfConfig lpf, QMI8658_StConfig st) {
    unsigned char data;

    switch (range) {
        case QMI8658GyrRange_32dps: gyro_lsb_div = 1024; break;
        case QMI8658GyrRange_64dps: gyro_lsb_div = 512; break;
        case QMI8658GyrRange_128dps: gyro_lsb_div = 256; break;
        case QMI8658GyrRange_256dps: gyro_lsb_div = 128; break;
        case QMI8658GyrRange_512dps: gyro_lsb_div = 64; break;
        case QMI8658GyrRange_1024dps: gyro_lsb_div = 32; break;
        case QMI8658GyrRange_2048dps: gyro_lsb_div = 16; break;
        case QMI8658GyrRange_4096dps: gyro_lsb_div = 8; break;
        default: gyro_lsb_div = 64; break;
    }

    data = range | odr;
    if (st == QMI8658St_Enable) data |= 0x80;
    QMI8658_write_reg(QMI8658Register_Ctrl3, data);

    QMI8658_read_reg(QMI8658Register_Ctrl5, &data, 1);
    data &= 0x0f;
    data |= (lpf == QMI8658Lpf_Enable) ? (G_LSP_MODE_3 | 0x10) : 0;
    QMI8658_write_reg(QMI8658Register_Ctrl5, data);
}

void QMI8658_config_mag(QMI8658_MagDev dev, QMI8658_MagOdr odr) {
    QMI8658_write_reg(QMI8658Register_Ctrl4, dev | odr);
}

void QMI8658_config_ae(QMI8658_AeOdr odr) {
    QMI8658_config_acc(QMI8658_config.accRange, QMI8658_config.accOdr, QMI8658Lpf_Enable, QMI8658St_Disable);
    QMI8658_config_gyro(QMI8658_config.gyrRange, QMI8658_config.gyrOdr, QMI8658Lpf_Enable, QMI8658St_Disable);
    QMI8658_config_mag(QMI8658_config.magDev, QMI8658_config.magOdr);
    QMI8658_write_reg(QMI8658Register_Ctrl6, odr);
}

void QMI8658_enableSensors(unsigned char flags) {
    if (flags & QMI8658_CONFIG_AE_ENABLE)
        flags |= QMI8658_CTRL7_ACC_ENABLE | QMI8658_CTRL7_GYR_ENABLE;
    QMI8658_write_reg(QMI8658Register_Ctrl7, flags & QMI8658_CTRL7_ENABLE_MASK);
}

void QMI8658_Config_apply(const QMI8658Config* config) {
    unsigned char input = config->inputSelection;

    if (input & QMI8658_CONFIG_AE_ENABLE) {
        QMI8658_config_ae(config->aeOdr);
    } else {
        if (input & QMI8658_CONFIG_ACC_ENABLE)
            QMI8658_config_acc(config->accRange, config->accOdr, QMI8658Lpf_Enable, QMI8658St_Disable);
        if (input & QMI8658_CONFIG_GYR_ENABLE)
            QMI8658_config_gyro(config->gyrRange, config->gyrOdr, QMI8658Lpf_Enable, QMI8658St_Disable);
    }

    if (input & QMI8658_CONFIG_MAG_ENABLE)
        QMI8658_config_mag(config->magDev, config->magOdr);

    QMI8658_enableSensors(input);
}

void QMI8658_read_xyz(float acc[3], float gyro[3], unsigned int* tim_count) {
    unsigned char buf[12];
    short raw_acc[3], raw_gyro[3];

    if (tim_count) {
        unsigned char t[3];
        QMI8658_read_reg(QMI8658Register_Timestamp_L, t, 3);
        *tim_count = ((uint32_t)t[2] << 16) | ((uint32_t)t[1] << 8) | t[0];
    }

    QMI8658_read_reg(QMI8658Register_Ax_L, buf, 12);

    for (int i = 0; i < 3; i++) {
        raw_acc[i] = (int16_t)((buf[2 * i + 1] << 8) | buf[2 * i]);
        raw_gyro[i] = (int16_t)((buf[2 * (i + 3) + 1] << 8) | buf[2 * (i + 3)]);
        acc[i] = (float)(raw_acc[i] * 1000.0f) / acc_lsb_div;
        gyro[i] = (float)(raw_gyro[i]) / gyro_lsb_div;
    }
}
