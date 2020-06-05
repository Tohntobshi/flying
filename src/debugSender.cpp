#include "debugSender.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

void DebugSender::sendInfo(float pitchError, float rollError, int64_t timestamp)
{
  Document d;
  Value& rootObj = d.SetObject();
  rootObj.AddMember("pitchErr", pitchError, d.GetAllocator());
  rootObj.AddMember("rollErr", rollError, d.GetAllocator());
  rootObj.AddMember("timestamp", timestamp, d.GetAllocator());
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  d.Accept(writer);
  std::string json = buffer.GetString();
  uint32_t size = json.size() + 1;
  uint8_t * buf = new uint8_t[size];
  memcpy(buf, json.c_str(), size);
  sendPacket({ buf, size }, IGNORE_IF_NO_CONN);
}
