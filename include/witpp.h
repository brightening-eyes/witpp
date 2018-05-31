/*
wit++
Copyright (c) 2018 amir ramezani

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#ifndef _WITPP_H
#define _WITPP_H
#pragma once

//include the headers
#include <ctime>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>
#include <functional>
#include <json/json.h>
#include <curl/curl.h>
#ifdef VAD_ENABLED
#include "vad.h"
#endif //VAD_ENABLED

namespace witpp
{

//this callback function gets called when a data is going to be received
size_t writecb(char* buf, size_t sz, size_t items, std::string* s)
{
size_t newLength = sz*items;
s->append(buf, newLength);
return newLength;
}

//this callback function is used for http put requests and reading data from std::string into char*
size_t readcb(char* buf, size_t sz, size_t items, void* s)
{
std::istream* stream=static_cast<std::istream*>(s);
stream->read(static_cast<char*>(buf), sz*items);
return stream->gcount();
}

//this class represents a value for context
class ContextValue
{
std::string value;
std::vector <std::string> expressions;
public:
ContextValue& setValue(std::string v)
{
value=v;
return *this;
}

std::string getValue()
{
return this->value;
}

ContextValue& addExpression(std::string expression)
{
expressions.push_back(expression);
return *this;
}

std::vector<std::string> getExpressions()
{
return expressions;
}

};

//this class represents an entity
class ContextEntity
{
std::string id;
std::vector<ContextValue> values;

public:

ContextEntity& setId(std::string i)
{
this->id=i;
return *this;
}

std::string getId()
{
return this->id;
}

ContextEntity& addValue(ContextValue& value)
{
values.push_back(value);
return *this;
}

std::vector<ContextValue> getValues()
{
return values;
}

};

//this class represents a context
class Context
{
private:
std::vector<std::string> states;
std::string referenceTime;
std::string timezone;
std::string locale;
std::vector<ContextEntity> entities;

public:
Context()
{
states.clear();
referenceTime="";
timezone="";
locale="";
entities.clear();
}

Context(std::string s)
{
std::stringstream stream;
stream.str(s);
Json::CharReaderBuilder builder;
Json::Value root;
std::string errors;
if(!Json::parseFromStream(builder, stream, &root, &errors))
{
throw std::invalid_argument(errors);
}
Json::Value st=root["state"];
if(!st.empty())
{
for(unsigned int i=0;i<st.size();i++)
{
addState(st[i].asString());
}
}
if(!root["reference_time"])
{
setReferenceTime(root["reference_time"].asString());
}
if(!root["timezone"])
{
setTimezone(root["timezone"].asString());
}
Json::Value ent=root["entities"];
if(!ent.empty())
{
for(unsigned int i=0;i<ent.size();i++)
{
ContextEntity e;
e.setId(ent[i]["id"].asString());
Json::Value vals=ent["values"];
for(unsigned int j=0;j<vals.size();j++)
{
ContextValue v;
v.setValue(vals[j]["value"].asString());
Json::Value expr=vals[j]["expressions"];
for(unsigned int exp=0;exp<expr.size();exp++)
{
v.addExpression(expr[exp].asString());
}
e.addValue(v);
}
addEntity(e);
}
}
if(!root["locale"].empty())
{
setLocale(root["locale"].asString());
}
}

operator std::string()
{
Json::StreamWriterBuilder builder;
std::unique_ptr<Json::StreamWriter> w(builder.newStreamWriter());
Json::Value root;
if(!states.empty())
{
Json::Value st(Json::ValueType::arrayValue);
for(unsigned int i=0;i<getStates().size();i++)
{
st.append(getStates()[i]);
}
root["state"]=st;
}
if(getReferenceTime()!="")
{
root["reference_time"]=getReferenceTime();
}
if(getTimezone()!="")
{
root["timezone"]=getTimezone();
}
if(!entities.empty())
{
Json::Value v(Json::ValueType::arrayValue);
for(unsigned int i=0;i<entities.size();i++)
{
Json::Value vals(Json::ValueType::objectValue);
vals["id"]=entities[i].getId();
Json::Value values(Json::ValueType::arrayValue);
for(unsigned int j=0;j<entities[i].getValues().size();j++)
{
values["value"]=entities[i].getValues()[j].getValue();
Json::Value expressions(Json::ValueType::arrayValue);
for(unsigned int k=0;k<entities[i].getValues()[j].getExpressions().size();k++)
{
expressions.append(entities[i].getValues()[j].getExpressions()[k]);
}
values["expressions"]=expressions;
}
v["values"]=values;
}
root["entities"]=v;
}
if(getLocale()!="")
{
root["locale"]=getLocale();
}
std::stringstream stream;
w->write(root, &stream);
return stream.str();
}

Context& addState(std::string state)
{
states.push_back(state);
return *this;
}

std::vector<std::string> getStates()
{
return states;
}

Context& setReferenceTime(std::string ref)
{
referenceTime=ref;
return *this;
}

std::string getReferenceTime()
{
return referenceTime;
}

Context& setTimezone(std::string tz)
{
timezone=tz;
return *this;
}

std::string getTimezone()
{
return this->timezone;
}

Context& addEntity(ContextEntity& e)
{
entities.push_back(e);
return *this;
}

std::vector<ContextEntity> getEntities()
{
return entities;
}

Context& setLocale(std::string l)
{
locale=l;
return *this;
}

std::string getLocale()
{
return locale;
}

};

//this class holds the parameters like authorization, version, etc
class Parameter
{
std::string version;
std::string server_token;
public:
Parameter()
{
time_t t=time(0);
struct tm n=*localtime(&t);
char buf[80];
strftime(buf, sizeof(buf), "%Y%m%d", &n);
version=buf;
}

Parameter& setVersion(std::string v)
{
version=v;
return *this;
}

std::string getVersion()
{
return version;
}

Parameter& setAuth(std::string auth)
{
server_token=auth;
return *this;
}

std::string getAuth()
{
return server_token;
}

};

//this class is an exception for wit.ai
class WitException: public std::exception
{
std::string errorText;
int errorCode;
public:
WitException(std::string e, int c):
errorText(e),
errorCode(c)
{

}

WitException(int code):
errorText(""),
errorCode(code)
{
switch(code)
{
case 400:
errorText="missing body (code: body) or missing content-type (code: content-type) or unknown content-type (code: unknown-content-type) or speech recognition failed (code: speech-rec) or invalid parameters (code: invalid-params)";
break;
case 401:
errorText="missing or wrong auth token (code: auth)";
break;
case 408:
errorText="request timed out, client was to slow to send data (code: timeout)";
break;
case 500:
errorText="something went wrong on our side, our experts are probably fixing it. (code: wit)";
break;
case 503:
errorText="something is very wrong on our side, our experts are probably being yelled at";
break;
default:
errorText="unknown error";
}
errorText="unknown exception";
}

virtual const char* what() const
{
return errorText.c_str();
}

int getCode()
{
return errorCode;
}

};

//this class represents a responce which the request return's it
class Responce
{
protected:
Json::Value responce;
public:

Responce(std::string r)
{
Json::CharReaderBuilder builder;
std::stringstream stream;
stream.str(r);
std::string errors;
if(!Json::parseFromStream(builder, stream, &responce, &errors))
{
throw std::invalid_argument(errors);
}
else
{
if(responce.isMember("error")||responce.isMember("code")||responce["error"].isString())
{
if(responce["code"].isInt()||responce["code"].isUInt())
{
throw WitException(responce["error"].asString(), responce["code"].asInt());
}
else
{
throw WitException(responce["error"].asString(), 0);
}
}
}
}

};

//this class is derived from Responce that shows a message (for MessageRequest and Speech Request)
class MessageResponce: public Responce
{
public:

MessageResponce(std::string s):
Responce(s)
{

}

std::string getMessageId()
{
return responce["msg_id"].asString();
}

std::string getText()
{
return responce["_text"].asString();
}

Json::Value getEntities()
{
return responce["entities"];
}

};

//this class represents a request
class Request
{
protected:
CURL* c;
CURLcode res;
struct curl_slist* headers;
std::string host;
Parameter param;
int timeout;

Request():
timeout(0)
{
host="https://api.wit.ai/";
c=curl_easy_init();
headers=curl_slist_append(nullptr, "Content-Type: application/json");
headers=curl_slist_append(headers, "Accept: application/json");
res=curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, writecb);
res=curl_easy_setopt(c, CURLOPT_READFUNCTION, readcb);
}

~Request()
{
curl_slist_free_all(headers);
curl_easy_cleanup(c);
timeout=0;
}

Request& setHost(std::string h)
{
host=h;
return *this;
}

std::string getHost()
{
return host;
}

public:
Request& setParameter(Parameter& p)
{
param=p;
return *this;
}

Parameter getParameter()
{
return param;
}

Request& setTimeout(int t)
{
timeout=t;
return *this;
}

int getTimeout()
{
return timeout;
}

};

//this class is a request for message
class MessageRequest: public Request
{
std::string thread_id;
std::string message_id;
std::string message;
Context* context;
bool has_context;
int n_best;
bool verbose;

public:
MessageRequest():
Request(),
verbose(false),
context(nullptr)
{
setHost(getHost()+"message");
n_best=1;
}

MessageRequest& setMessage(std::string m)
{
message=m;
return *this;
}

std::string getMessage()
{
return message;
}

MessageRequest& setMessageId(std::string id)
{
message_id=id;
return *this;
}

std::string getMessageId()
{
return message_id;
}

MessageRequest& setThreadId(std::string id)
{
thread_id=id;
return *this;
}

std::string getThreadId()
{
return thread_id;
}

MessageRequest& setContext(Context* c)
{
context=c;
return *this;
}

Context getContext()
{
return *context;
}

MessageRequest& setNBest(int n)
{
n_best=n;
return *this;
}

int getNBest()
{
return n_best;
}

MessageRequest&setVerbose(bool v)
{
verbose=v;
return *this;
}

bool getVerbose()
{
return verbose;
}

MessageResponce perform()
{
if(message=="")
{
throw WitException("message field should not be empty", 0);
}
std::string s="";
s+="?v="+param.getVersion();
if(context!=nullptr)
{
std::string ctx=(std::string)*context;
s+="&context="+(std::string)curl_escape(ctx.c_str(), ctx.length());
}
if(message_id!="")
{
s+="&msg_id="+(std::string)curl_escape(message_id.c_str(), message_id.length());
}
if(thread_id!="")
{
s+="&thread_id="+(std::string)curl_escape(thread_id.c_str(), thread_id.length());
}
s+="&n="+n_best;
s+="&verbose="+verbose?"1":"0";
s+="&q="+(std::string)curl_escape(message.c_str(), message.length());
std::string token_header="Authorization: Bearer "+param.getAuth();
headers=curl_slist_append(headers, token_header.c_str());
std::string url=host+s;
res=curl_easy_setopt(c, CURLOPT_URL, url.c_str());
res=curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers);
res=curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
if(getTimeout()!=0)
{
res=curl_easy_setopt(c, CURLOPT_TIMEOUT, getTimeout());
}
std::unique_ptr<std::string> data(new std::string());;
res=curl_easy_setopt(c, CURLOPT_WRITEDATA, data.get());
res=curl_easy_perform(c);
int httpcode;
curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &httpcode);
if(httpcode==200)
{
return MessageResponce(*data.get());
}
else
{
throw WitException(httpcode);
}
}

};

#ifdef VAD_ENABLED

class VoiceActivityDetector
{
wvs_state* state;
public:
VoiceActivityDetector(int sample_rate)
{
state=wvs_init(0, sample_rate);
}

~VoiceActivityDetector()
{
wvs_clean(state);
}

bool talking(int16_t* samples, int size)
{
if(wvs_still_talking(state, samples, size)==1)
{
return true;
}
return false;
}

};

#endif //VAD_ENABLED

typedef enum
{
BIG_ENDIAN,
LITTLE_ENDIAN
}EndianType;

typedef enum
{
BIT_TYPE_8BIT,
BIT_TYPE_16BIT,
BIT_TYPE_32BIT
}BitsType;;

typedef enum
{
SIGNED_INTEGER,
UNSIGNED_INTEGER,
FLOATING_POINT,
MU_LAW,
A_LAW,
IMA_ADPCM,
MS_ADPCM,
GSM_FULL_RATE
}EncodingType;

typedef enum {
RECORDING_STARTED,
RECORDING_CONTINUE,
RECORDING_STOPPED
}RecordingStatus;

typedef std::function<RecordingStatus (std::stringstream&)> SourceFunction;

class VoiceRequest: public Request
{
SourceFunction callback;
int rate;
std::string thread_id;
std::string message_id;
Context* context;
int n_best;
bool verbose;
public:

VoiceRequest():
Request(),
verbose(false),
context(nullptr)
{
headers=curl_slist_append(headers, "Content-Type: audio/raw");
headers=curl_slist_append(headers, "endian: little");
headers=curl_slist_append(headers, "bits: 16");
headers=curl_slist_append(headers, "encoding: unsigned-integer");
setHost(getHost()+"speech");
n_best=1;
rate=16000;
}

VoiceRequest& setEndian(EndianType endian)
{
switch(endian)
{
case BIG_ENDIAN:
headers=curl_slist_append(headers, "endian: big");
break;
case LITTLE_ENDIAN:
headers=curl_slist_append(headers, "endian: little");
break;
}
return *this;
}

VoiceRequest& setBitType(BitsType type)
{
switch(type)
{
case BIT_TYPE_8BIT:
headers=curl_slist_append(headers, "bits: 8");
break;
case BIT_TYPE_16BIT:
headers=curl_slist_append(headers, "bits: 16");
break;
case BIT_TYPE_32BIT:
headers=curl_slist_append(headers, "bits: 32");
break;
}
return *this;
}

VoiceRequest& setEncoding(EncodingType type)
{
switch(type)
{
case SIGNED_INTEGER:
headers=curl_slist_append(headers, "encoding: signed-integer");
break;
case UNSIGNED_INTEGER:
headers=curl_slist_append(headers, "encoding: unsigned-integer");
break;
case FLOATING_POINT:
headers=curl_slist_append(headers, "encoding: floating-point");
break;
case MU_LAW:
headers=curl_slist_append(headers, "encoding: mu-law");
break;
case A_LAW:
headers=curl_slist_append(headers, "encoding: A_LAW");
break;
case IMA_ADPCM:
headers=curl_slist_append(headers, "encoding: ima-adpcm");
break;
case MS_ADPCM:
headers=curl_slist_append(headers, "encoding: ms-adpcm");
break;
case GSM_FULL_RATE:
headers=curl_slist_append(headers, "encoding: gsm-full-rate");
break;
}
return *this;
}

VoiceRequest& setSourceCallback(SourceFunction cb)
{
callback=cb;
return *this;
}

SourceFunction getSourceCallback()
{
return callback;
}

VoiceRequest& setSampleRate(int sample_rate)
{
rate=sample_rate;
}

int getSampleRate()
{
return rate;
}

VoiceRequest& setMessageId(std::string id)
{
message_id=id;
return *this;
}

std::string getMessageId()
{
return message_id;
}

VoiceRequest& setThreadId(std::string id)
{
thread_id=id;
return *this;
}

std::string getThreadId()
{
return thread_id;
}

VoiceRequest& setContext(Context* c)
{
context=c;
return *this;
}

Context getContext()
{
return *context;
}

VoiceRequest& setNBest(int n)
{
n_best=n;
return *this;
}

int getNBest()
{
return n_best;
}

VoiceRequest&setVerbose(bool v)
{
verbose=v;
return *this;
}

bool getVerbose()
{
return verbose;
}

MessageResponce perform()
{
std::string s="";
s+="?v="+param.getVersion();
if(context!=nullptr)
{
std::string ctx=(std::string)*context;
s+="&context="+(std::string)curl_escape(ctx.c_str(), ctx.length());
}
if(message_id!="")
{
s+="&msg_id="+(std::string)curl_escape(message_id.c_str(), message_id.length());
}
if(thread_id!="")
{
s+="&thread_id="+(std::string)curl_escape(thread_id.c_str(), thread_id.length());
}
s+="&n="+n_best;
s+="&verbose="+verbose?"1":"0";
std::string token_header="Authorization: Bearer "+param.getAuth();
headers=curl_slist_append(headers, token_header.c_str());
std::string url=host+s;
res=curl_easy_setopt(c, CURLOPT_URL, url.c_str());
res=curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers);
res=curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
if(getTimeout()!=0)
{
res=curl_easy_setopt(c, CURLOPT_TIMEOUT, getTimeout());
}
int r=RECORDING_STARTED;
std::stringstream stream;
do
{
r=callback(stream);
if(r==RECORDING_CONTINUE)
{
stream.flush();
}
}while(r!=RECORDING_STOPPED);
res=curl_easy_setopt(c, CURLOPT_POST, 1L);
std::string str=stream.str();
res=curl_easy_setopt(c, CURLOPT_POSTFIELDS, str.c_str());
std::unique_ptr<std::string> data(new std::string());;
res=curl_easy_setopt(c, CURLOPT_WRITEDATA, data.get());
res=curl_easy_perform(c);
int httpcode;
curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &httpcode);
if(httpcode==200)
{
return MessageResponce(*data.get());
}
else
{
throw WitException(httpcode);
}
}

};

//this class represents a responce for getting entities
class EntitiesResponce: public Responce
{
public:

EntitiesResponce(std::string s):
Responce(s)
{

}

std::vector<std::string> getEntities()
{
std::vector<std::string> data;
for(unsigned int i=0;i<responce.size();i++)
{
data.push_back(responce[i].asString());
}
return data;
}

};

//this class represents a request for getting entities
class EntitiesRequest: public Request
{
public:
EntitiesRequest():
Request()
{
setHost(getHost()+"entities");
}

EntitiesResponce perform()
{
std::string s="";
s+="?v="+param.getVersion();
std::string token_header="Authorization: Bearer "+param.getAuth();
headers=curl_slist_append(headers, token_header.c_str());
std::string url=host+s;
res=curl_easy_setopt(c, CURLOPT_URL, url.c_str());
res=curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers);
res=curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
if(getTimeout()!=0)
{
res=curl_easy_setopt(c, CURLOPT_TIMEOUT, getTimeout());
}
std::unique_ptr<std::string> data(new std::string());;
res=curl_easy_setopt(c, CURLOPT_WRITEDATA, data.get());
res=curl_easy_perform(c);
int httpcode;
curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &httpcode);
if(httpcode==200)
{
return EntitiesResponce(*data.get());
}
else
{
throw WitException(httpcode);
}
}

};

//this class is for getting responce from create/update entities
class EntityResponce: public Responce
{
public:
EntityResponce(std::string e):
Responce(e)
{

}

std::string getName()
{
return responce["name"].asString();
}

std::string getLanguage()
{
return responce["lang"].asString();
}

std::vector<std::string> getLookups()
{
std::vector<std::string> dt;
Json::Value r=responce["lookups"];
for(unsigned int i=0;i<r.size();i++)
{
dt.push_back(r[i].asString());
}
return dt;
}

bool isBuiltin()
{
return responce["builtin"].asBool();
}

std::string getDocumentation()
{
return responce["doc"].asString();
}

std::string getId()
{
return responce["id"].asString();
}

};

//this class is for creating new entities
class CreateEntityRequest: public Request
{
std::string id;
std::string doc;
public:
CreateEntityRequest():
Request()
{
setHost(getHost()+"entities");
}

CreateEntityRequest& setId(std::string i)
{
id=i;
return *this;
}

std::string getId()
{
return id;
}

CreateEntityRequest& setDocumentation(std::string d)
{
doc=d;
return *this;
}

std::string getDocumentation()
{
return doc;
}

EntityResponce perform()
{
if(id=="")
{
throw WitException("the id field is mandatory", 0);
}
Json::Value root;
root["id"]=id;
root["doc"]=doc;
Json::StreamWriterBuilder builder;
std::unique_ptr<Json::StreamWriter> w(builder.newStreamWriter());
std::stringstream stream;
w->write(root, &stream);
std::string s="";
s+="?v="+param.getVersion();
std::string token_header="Authorization: Bearer "+param.getAuth();
headers=curl_slist_append(headers, token_header.c_str());
std::string url=host+s;
res=curl_easy_setopt(c, CURLOPT_URL, url.c_str());
res=curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers);
res=curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
if(getTimeout()!=0)
{
res=curl_easy_setopt(c, CURLOPT_TIMEOUT, getTimeout());
}
res=curl_easy_setopt(c, CURLOPT_POST, 1L);
std::string postdata=stream.str();
res=curl_easy_setopt(c, CURLOPT_POSTFIELDS, postdata.c_str());
std::unique_ptr<std::string> data(new std::string());;
res=curl_easy_setopt(c, CURLOPT_WRITEDATA, data.get());
res=curl_easy_perform(c);
int httpcode;
curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &httpcode);
if(httpcode==200)
{
return EntityResponce(*data.get());
}
else
{
throw WitException(httpcode);
}
}

};

//this class is for values of an entity when we get it in the responce
class EntityValue
{
std::string value;
std::vector<std::string> expressions;
public:
EntityValue(std::string v, std::vector<std::string> expr)
{
value=v;
expressions=expr;
}

std::string getValue()
{
return value;
}

std::vector<std::string> getExpressions()
{
return expressions;
}

};

//this class is used for getting information about an entity
class EntityValueResponce: public Responce
{
public:
EntityValueResponce(std::string r):
Responce(r)
{

}

bool isBuiltin()
{
return responce["builtin"].asBool();
}

std::string getDocumentation()
{
return responce["doc"].asString();
}

std::string getId()
{
return responce["id"].asString();
}

std::string getLanguage()
{
return responce["lang"].asString();
}

std::vector<std::string> getLookups()
{
std::vector<std::string> data;
Json::Value lookups=responce["lookups"];
for(unsigned int i=0;i<lookups.size();i++)
{
data.push_back(lookups[i].asString());
}
return data;
}

std::string getName()
{
return responce["name"].asString();
}

std::vector<EntityValue> getValues()
{
std::vector<EntityValue> vals;
Json::Value v=responce["values"];
for(unsigned int i=0;i<v.size();i++)
{
Json::Value val=v[i];
for(unsigned int j=0;j<val.size();j++)
{
std::vector<std::string> ex;
Json::Value expressions=val["expressions"];
for(unsigned int x=0;x<expressions.size();x++)
{
ex.push_back(expressions[x].asString());
}
EntityValue e(val["value"].asString(), ex);
vals.push_back(e);
}
}
return vals;
}

};

//this class represents value for UpdateEntityRequest
class UpdateEntityValue
{
std::string value;
std::vector<std::string> expressions;
Json::Value metadata;
public:
UpdateEntityValue(std::string val):
value(val)
{

}

UpdateEntityValue(std::string val, std::vector<std::string> expr):
value(val),
expressions(expr)
{

}

UpdateEntityValue(std::string val, std::vector<std::string> expr, Json::Value m):
value(val),
expressions(expr),
metadata(m)
{

}

UpdateEntityValue& setValue(std::string val)
{
value=val;
return *this;
}

std::string getValue()
{
return value;
}

UpdateEntityValue& addExpression(std::string expression)
{
expressions.push_back(expression);
return *this;
}

UpdateEntityValue& setExpressions(std::vector<std::string> expr)
{
expressions=expr;
return *this;
}

std::vector<std::string> getExpressions()
{
return expressions;
}

UpdateEntityValue& setMetadata(Json::Value m)
{
metadata=m;
return *this;
}

Json::Value getMetadata()
{
return metadata;
}

};

//this class is for updating an entity
class UpdateEntityRequest: public Request
{
std::string id;
std::string doc;
std::vector<UpdateEntityValue> values;
public:
UpdateEntityRequest(std::string name):
Request()
{
setHost(getHost()+"entities/"+name);
}

UpdateEntityRequest& setId(std::string i)
{
id=i;
return *this;
}

std::string getId()
{
return id;
}

UpdateEntityRequest& setDocumentation(std::string d)
{
doc=d;
return *this;
}

std::string getDocumentation()
{
return doc;
}

UpdateEntityRequest& addValue(UpdateEntityValue v)
{
values.push_back(v);
return *this;
}

UpdateEntityRequest& setValues(std::vector<UpdateEntityValue> val)
{
values=val;
return *this;
}

std::vector<UpdateEntityValue> getValues()
{
return values;
}

EntityResponce perform()
{
Json::Value root;
root["id"]=id;
root["doc"]=doc;
Json::Value vals(Json::ValueType::arrayValue);
for(unsigned int i=0;i<values.size();i++)
{
Json::Value val(Json::ValueType::objectValue);
val["value"]=values[i].getValue();
Json::Value exprs(Json::ValueType::arrayValue);
std::vector<std::string> e=values[i].getExpressions();
for(unsigned int j=0;j<e.size();j++)
{
exprs.append(e[j]);
}
val["expressions"]=exprs;
val["metadata"]=values[i].getMetadata();
vals.append(val);
}
root["values"]=vals;
Json::StreamWriterBuilder builder;
std::unique_ptr<Json::StreamWriter> w(builder.newStreamWriter());
std::unique_ptr<std::stringstream> stream(new std::stringstream());;
w->write(root, stream.get());
std::string s="";
s+="?v="+param.getVersion();
std::string token_header="Authorization: Bearer "+param.getAuth();
headers=curl_slist_append(headers, token_header.c_str());
std::string url=host+s;
res=curl_easy_setopt(c, CURLOPT_URL, url.c_str());
res=curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers);
res=curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
if(getTimeout()!=0)
{
res=curl_easy_setopt(c, CURLOPT_TIMEOUT, getTimeout());
}
res=curl_easy_setopt(c, CURLOPT_UPLOAD, 1L);
res=curl_easy_setopt(c, CURLOPT_READDATA, stream.get());
std::unique_ptr<std::string> data(new std::string());
res=curl_easy_setopt(c, CURLOPT_WRITEDATA, data.get());
res=curl_easy_perform(c);
int httpcode;
curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &httpcode);
if(httpcode==200)
{
return EntityResponce(*data.get());
}
else
{
throw WitException(httpcode);
}
}

};

}

#endif //_WITPP_H
