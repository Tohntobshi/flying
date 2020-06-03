#include "frameSender.h"

void FrameSender::beforeLoop()
{
  frameCapturer = FrameCapturer::Init();
  frameCapturer->initEncDecLib();
  frameCapturer->initCamera();
  frameCapturer->initEncoding([&](uint8_t *buf, uint32_t size) {
    if (!conn.sendPacket(buf, size)) sendError = true;
  });
}

void FrameSender::iterate()
{
  if (!conn.listenIP("8080")) {
  // if (!conn.listenBT(9)) {
    std::cout << "frame sender: not listening, retry in 3 seconds...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    return;
  }
  sendError = false;
  while(!shouldStop())
  {
    frameCapturer->captureEncodedFrame();
    if (sendError) return;
  }
}

void FrameSender::afterLoop()
{
  frameCapturer->Destroy();
}

void FrameSender::onStop()
{
  conn.abortListenAndReceive();
}
