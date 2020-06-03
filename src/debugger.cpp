#include "debugger.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

void Debugger::sendInfo(float pitchError, float rollError, int64_t timestamp)
{
  if (!conn.isConnected()) return;
  Document d;
  Value& rootObj = d.SetObject();
  rootObj.AddMember("pitchErr", pitchError, d.GetAllocator());
  rootObj.AddMember("rollErr", rollError, d.GetAllocator());
  rootObj.AddMember("timestamp", timestamp, d.GetAllocator());
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  d.Accept(writer);
  std::string json = buffer.GetString();
  // std::cout << json << "\n";
  std::unique_lock<std::mutex> lck(dataToSendMtx);
  debugDataToSend.push_back(json);
}

void Debugger::iterate()
{
  if (!conn.listenIP("8082")) {
  // if (!conn.listenBT(9)) {
    std::cout << "send debug: not listening, retry in 3 seconds...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    return;
  }
  while(!shouldStop())
  {
    std::unique_lock<std::mutex> lck(dataToSendMtx);
    if (debugDataToSend.size() > 0)
    {
      std::string data = debugDataToSend.front();
      debugDataToSend.pop_front();
      lck.unlock();
      uint32_t size = data.size() + 1;
      uint8_t * buf = new uint8_t[size];
      memcpy(buf, data.c_str(), size);
      bool sent = conn.sendPacket(buf, size);
      delete [] buf;
      if (!sent) return;
    }
    else
    {
      lck.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  }
}

void Debugger::onStop()
{
  conn.abortListenAndReceive();
}

Debugger::Debugger()
{

}