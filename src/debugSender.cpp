#include "debugSender.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>

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
  const char * json = buffer.GetString();
  uint32_t size = buffer.GetSize();
  uint8_t * buf = new uint8_t[size + 1];
  memcpy(buf, json, size);
  buf[size] = 0;
  sendPacket({ buf, size + 1 }, IGNORE_IF_NO_CONN);
}
