public:
  void setHeadless(bool h); // prevents any internal drawing
  void onSample(void (*cb)(float r, float g, float b, float c));

private:
  bool headless = false;
  void (*sampleCB)(float, float, float, float) = nullptr;
