/*
**
** Author(s):
**  - Pierre ROULLON <proullon@aldebaran-robotics.com>
**
** Copyright (C) 2012, 2013 Aldebaran Robotics
** See COPYING for the license
*/


#include <boost/locale.hpp>

#include <qi/log.hpp>
#include <qi/signature.hpp>
#include <qi/type/dynamicobjectbuilder.hpp>
#include <qi/anyobject.hpp>
#include <qi/anyvalue.hpp>
#include <qi/type/typedispatcher.hpp>
#include <qi/type/typeinterface.hpp>

#include <jnitools.hpp>
#include <jobjectconverter.hpp>
#include <map_jni.hpp>
#include <list_jni.hpp>
#include <tuple_jni.hpp>
#include <object_jni.hpp>
#include <future_jni.hpp>

qiLogCategory("qimessaging.jni");
using namespace qi;

static const int JAVA_INT_NBYTES = 4;

struct toJObject
{
    toJObject(jobject *result)
      : result(result)
    {
      env = attach.get();
    }

    void visitUnknown(qi::AnyReference value)
    {
      throwNewException(env, "Error in conversion: Unable to convert unknown type in Java");
    }

    void visitInt(qi::int64_t value, bool isSigned, int byteSize)
    {
      qiLogVerbose() << "visitInt " << value << ' ' << byteSize;
      // Clear all remaining exceptions
      env->ExceptionClear();

      if (byteSize == 0)
        *result = qi::jni::construct(env, "java/lang/Boolean", "(Z)V", static_cast<jboolean>(value));
      else if (byteSize <= JAVA_INT_NBYTES)
        *result = qi::jni::construct(env, "java/lang/Integer", "(I)V", static_cast<jint>(value));
      else
        *result = qi::jni::construct(env, "java/lang/Long", "(J)V", static_cast<jlong>(value));
      checkForError();
    }

    void visitString(char *data, size_t len)
    {
      qiLogVerbose() << "visitString " << len;
      if (data)
      {
        // It is unclear wether wstring and wchar_t are garanteed 16 bytes,
        std::basic_string<jchar> conv = boost::locale::conv::utf_to_utf<jchar>(data, data+len);
        if (conv.empty())
          *result = (jobject) env->NewStringUTF("");
        else
          *result = (jobject) env->NewString(&conv[0], conv.length());
      }
      else
        *result = (jobject) env->NewStringUTF("");
      checkForError();
    }

    void visitVoid()
    {
      jclass cls = env->FindClass("java/lang/Void");
      jmethodID mid = env->GetMethodID(cls, "<init>", "()V");
      *result = env->NewObject(cls, mid);
      qi::jni::releaseClazz(cls);
      checkForError();
    }

    void visitFloat(double value, int byteSize)
    {
      qiLogVerbose() << "visitFloat " << value;
      // Clear all remaining exceptions
      env->ExceptionClear();

      // Find constructor method ID
      jmethodID mid = env->GetMethodID(cls_float, "<init>","(F)V");
      if (!mid)
      {
        throwNewException(env, "AnyValue to Float : GetMethodID error");
        return;
      }

      // Instanciate new Float, yeah !
      jfloat jval = value;
      *result = env->NewObject(cls_float, mid, jval);
      checkForError();
    }

    void visitList(qi::AnyIterator it, qi::AnyIterator end)
    {
      JNIList list; // this is OK.

      for(; it != end; ++it)
      {
        qi::AnyReference arRes = *it;
        std::pair<qi::AnyReference, bool> converted = arRes.convert(qi::typeOf<jobject>());
        jobject result = *(jobject*)converted.first.rawValue();
        list.push_back(result);
        if (converted.second)
          converted.first.destroy();
      }

      *result = list.object();
    }

    void visitMap(qi::AnyIterator it, qi::AnyIterator end)
    {
      JNIMap map;

      for (; it != end; ++it)
      {
        std::pair<qi::AnyReference, bool> keyConv =
          (*it)[0].convert(qi::typeOf<jobject>());
        std::pair<qi::AnyReference, bool> valConv =
          (*it)[1].convert(qi::typeOf<jobject>());

        map.put(*(jobject*)keyConv.first.rawValue(),
            *(jobject*)valConv.first.rawValue());

        if (keyConv.second)
          keyConv.first.destroy();
        if (valConv.second)
          valConv.first.destroy();
      }

      *result = map.object();
    }

    void visitObject(qi::GenericObject obj)
    {
      throw std::runtime_error("Cannot convert GenericObject to Jobject.");
    }

    void visitAnyObject(qi::AnyObject o)
    {
      try
      {
        JNIObject obj(o);
        *result = obj.object();
      }
      catch (std::exception&)
      {
      }
    }

    void visitPointer(qi::AnyReference pointee)
    {
      qiLogFatal() << "Error in conversion: Unable to convert pointer in Java";
      throwNewException(env, "Error in conversion: Unable to convert pointer in Java");
    }

private:
    jobject newTuple(jobjectArray values)
    {
      jmethodID ctor = env->GetMethodID(cls_tuple, "<init>", "([Ljava/lang/Object;)V");
      if (!ctor)
      {
        qiLogError() << "Cannot find Tuple constructor";
        return nullptr;
      }
      jobject result = env->NewObject(cls_tuple, ctor, values);
      return result;
    }

public:
    void visitTuple(const std::string& className, const std::vector<qi::AnyReference>& values, const std::vector<std::string>& annotations)
    {
      jobjectArray array = qi::jni::toJobjectArray(values);
      *result = newTuple(array);
      env->DeleteLocalRef(array);
    }

    void visitDynamic(qi::AnyReference pointee)
    {
      qiLogVerbose() << "visitDynamic";
      *result = JObject_from_AnyValue(pointee);
    }

    void visitRaw(qi::AnyReference value)
    {
      qiLogVerbose() << "visitRaw";
      qi::Buffer buf = value.as<qi::Buffer>();

      // Create a new ByteBuffer and reserve enough space
      jclass cls = env->FindClass("java/nio/ByteBuffer");
      jmethodID mid = env->GetStaticMethodID(cls, "allocate", "(I)Ljava/nio/ByteBuffer;");
      jobject ar = env->CallStaticObjectMethod(cls, mid, buf.size());

      // Put qi::Buffer content into a byte[] object
      const jbyte* data = (const jbyte*) buf.data();
      jbyteArray byteArray = env->NewByteArray(buf.size());
      env->SetByteArrayRegion(byteArray, 0, buf.size(), data);

      // Put the byte[] object into the ByteBuffer
      mid = env->GetMethodID(cls, "put","([BII)Ljava/nio/ByteBuffer;");
      *result = env->CallObjectMethod(ar, mid, byteArray, 0, buf.size());
      checkForError();
      env->DeleteLocalRef(cls);
      env->DeleteLocalRef(ar);
      env->DeleteLocalRef(byteArray);
    }

    void visitIterator(qi::AnyReference v)
    {
      visitUnknown(v);
    }

    void visitVarArgs(qi::AnyIterator begin, qi::AnyIterator end)
    {
      throw std::runtime_error("var args is not supported");
    }

    void checkForError()
    {
      if (result == NULL)
        throwNewException(env, "Error in conversion to JObject");
    }

    jobject* result;
    JNIEnv*  env;
    qi::jni::JNIAttach attach;

}; // !toJObject

jobject JObject_from_AnyValue(qi::AnyReference val)
{
  if (!val.isValid())
  {
    // We stored a null value, typeDispatch would be unhappy, so directly return nullptr here
    return nullptr;
  }
  jobject result= NULL;
  toJObject tjo(&result);
  qi::typeDispatch<toJObject>(tjo, val);
  return result;
}

void JObject_from_AnyValue(qi::AnyReference val, jobject* target)
{
  toJObject tal(target);
  qi::typeDispatch<toJObject>(tal, val);
}

qi::AnyReference AnyValue_from_JObject_List(jobject val)
{
  JNIEnv* env;
  JNIList list(val);
  std::vector<qi::AnyValue>& res = *new std::vector<qi::AnyValue>();
  int size = 0;

  JVM()->GetEnv((void **) &env, QI_JNI_MIN_VERSION);

  size = list.size();
  res.reserve(size);
  for (int i = 0; i < size; i++)
  {
    jobject current = list.get(i);
    std::pair<qi::AnyReference, bool> conv = AnyValue_from_JObject(current);
    res.push_back(qi::AnyValue(conv.first, !conv.second, true));
  }

  return qi::AnyReference::from(res);
}

qi::AnyReference AnyValue_from_JObject_Map(jobject hashmap)
{
  JNIEnv* env;
  std::map<qi::AnyValue, qi::AnyValue>& res = *new std::map<qi::AnyValue, qi::AnyValue>();
  JNIMap map(hashmap);

  JVM()->GetEnv((void **) &env, QI_JNI_MIN_VERSION);

  jobjectArray keys = map.keys();
  int size = env->GetArrayLength(keys);
  for (int i = 0; i < size; ++i)
  {
    jobject key = env->GetObjectArrayElement(keys, i);
    jobject value = map.get(key);
    std::pair<qi::AnyReference, bool> convKey = AnyValue_from_JObject(key);
    std::pair<qi::AnyReference, bool> convValue = AnyValue_from_JObject(value);
    res[qi::AnyValue(convKey.first, !convKey.second, true)] = qi::AnyValue(convValue.first, !convValue.second, true);
  }
  return qi::AnyReference::from(res);
}

qi::AnyReference AnyValue_from_JObject_Tuple(jobject val)
{
  JNITuple tuple(val);
  int i = 0;
  std::vector<qi::AnyReference> elements;
  std::vector<qi::AnyReference> toFree;
  while (i < tuple.size())
  {
    std::pair<qi::AnyReference, bool> convValue = AnyValue_from_JObject(tuple.get(i));
    elements.push_back(convValue.first);
    if (convValue.second)
      toFree.push_back(convValue.first);
    i++;
  }
  qi::AnyReference res = qi::makeGenericTuple(elements); // copies
  for (unsigned i=0; i<toFree.size(); ++i)
    toFree[i].destroy();
  return res;
}

/**
 * Make AnyReference from a Java Future object.
 * Future objects have a JNI member (a qi::Future) that we can
 * directly rely on, hence the use of the JNI Environment.
 * @param val The Future object.
 * @param env The JNI Environment.
 * @return
 */
qi::AnyReference AnyValue_from_JObject_Future(jobject val, JNIEnv* env)
{
  auto fieldId = env->GetFieldID(cls_future, "_fut", "J");
  auto futureAddress = env->GetLongField(val, fieldId);
  auto future = reinterpret_cast<qi::Future<qi::AnyValue>*>(futureAddress);

  // like done with the other types, we store the real data somewhere for the
  // reference to survive
  auto& futureCopy = *new qi::Future<qi::AnyValue>(*future);
  return qi::AnyReference::from(futureCopy);
}

qi::AnyReference AnyValue_from_JObject_RemoteObject(jobject val)
{
  JNIObject obj(val);

  qi::AnyObject* tmp = new qi::AnyObject();
  *tmp = obj.objectPtr();
  return qi::AnyReference::from(*tmp);
}

qi::AnyReference _AnyValue_from_JObject(jobject val);

std::pair<qi::AnyReference, bool> AnyValue_from_JObject(jobject val)
{
  if (!val)
    return std::make_pair(qi::AnyReference(), false);

  return std::make_pair(_AnyValue_from_JObject(val), true);
}

qi::AnyReference _AnyValue_from_JObject(jobject val)
{
  qi::AnyReference res;
  qi::jni::JNIAttach attach;
  JNIEnv *env = attach.get();

  if (env->IsInstanceOf(val, cls_string))
  {
    std::string tmp = qi::jni::toString(reinterpret_cast<jstring>(val));
    return qi::AnyReference::from(tmp).clone();
  }

  if (env->IsInstanceOf(val, cls_float))
  {
    jmethodID mid = env->GetMethodID(cls_float, "floatValue", "()F");
    jfloat v = env->CallFloatMethod(val, mid);
    return qi::AnyReference::from(v).clone();
  }

  if (env->IsInstanceOf(val, cls_double)) // If double, convert to float
  {
    jmethodID mid = env->GetMethodID(cls_double, "doubleValue", "()D");
    jfloat v = static_cast<jfloat>(env->CallDoubleMethod(val, mid));
    return qi::AnyReference::from(v).clone();
  }

  if (env->IsInstanceOf(val, cls_long))
  {
    jmethodID mid = env->GetMethodID(cls_long, "longValue", "()J");
    jlong v = env->CallLongMethod(val, mid);
    return qi::AnyReference::from(v).clone();
  }

  if (env->IsInstanceOf(val, cls_boolean))
  {
    jmethodID mid = env->GetMethodID(cls_boolean, "booleanValue", "()Z");
    jboolean v = env->CallBooleanMethod(val, mid);
    return qi::AnyReference::from(static_cast<bool>(v)).clone();
  }

  if (env->IsInstanceOf(val, cls_integer))
  {
    jmethodID mid = env->GetMethodID(cls_integer, "intValue", "()I");
    jint v = env->CallIntMethod(val, mid);
    return qi::AnyReference::from(v).clone();
  }

  if (env->IsInstanceOf(val, cls_list))
  {
    return AnyValue_from_JObject_List(val);
  }

  if (env->IsInstanceOf(val, cls_map))
  {
    return AnyValue_from_JObject_Map(val);
  }

  if (env->IsInstanceOf(val, cls_tuple))
  {
    return AnyValue_from_JObject_Tuple(val);
  }

  if (env->IsInstanceOf(val, cls_future))
  {
    return AnyValue_from_JObject_Future(val, env);
  }

  if (env->IsInstanceOf(val, cls_anyobject))
  {
    return AnyValue_from_JObject_RemoteObject(val);
  }
  qiLogError() << "Cannot serialize return value: Unable to convert JObject to AnyValue";
  throw std::runtime_error("Cannot serialize return value: Unable to convert JObject to AnyValue");
}


/*
 * Define this struct to add jobject to the type system.
 * That way we can manipulate jobject transparently.
 * - We have to override clone and destroy here to be compliant
 *   with the java reference counting. Otherwise, the value could
 *   be Garbage Collected as we try to manipulate it.
 * - We register the type as 'jobject' since java methods manipulates
 *   objects only by this typedef pointer, never by value and we do not want to copy
 *   a jobject.
 */
class JObjectTypeInterface: public qi::DynamicTypeInterface
{
  public:

    virtual const qi::TypeInfo& info()
    {
      static qi::TypeInfo* result = 0;
      if (!result)
        result = new qi::TypeInfo(typeid(jobject));

      return *result;
    }

    virtual void* initializeStorage(void* ptr = 0)
    {
      // ptr is jobject* (aka _jobject**)
      if (!ptr)
      {
        ptr = new jobject;
        *(jobject*)ptr = NULL;
      }
      return ptr;
    }

    virtual void* ptrFromStorage(void** s)
    {
      jobject** tmp = (jobject**) s;
      return *tmp;
    }

    virtual qi::AnyReference get(void* storage)
    {
      static unsigned int MEMORY_SIZE = 200;
      static bool init = false;
      static qi::AnyReference* memoryBuffer;
      if (!init)
      {
        /* This is such an awful hack, we'd rather provide a way to
        * control it from outside: 0 means disable cleanup entirely.
        */
        init = true;
        std::string v = qi::os::getenv("QI_JAVA_REFERENCE_POOL_SIZE");
        if (!v.empty())
          MEMORY_SIZE = strtol(v.c_str(), 0, 0);
        if (MEMORY_SIZE)
          memoryBuffer = new qi::AnyReference[MEMORY_SIZE];
      }
      static unsigned int memoryPosition = 0;
      std::pair<qi::AnyReference, bool> convValue = AnyValue_from_JObject(*((jobject*)ptrFromStorage(&storage)));
      if (convValue.second && MEMORY_SIZE)
      {
        memoryBuffer[memoryPosition].destroy();
        memoryBuffer[memoryPosition] = convValue.first;
        memoryPosition = (memoryPosition+1) % MEMORY_SIZE;
      }
      return convValue.first;
    }

    virtual void set(void** storage, qi::AnyReference src)
    {
      // storage is jobject**
      jobject* target = *(jobject**)storage;

      JNIEnv *env;
      qi::jni::JNIAttach attach;
      env = attach.get();

      if (*target)
        env->DeleteGlobalRef(*target);

      // Giving jobject* to JObject_from_AnyValue
      JObject_from_AnyValue(src, target);

      if (*target)
      {
        // create a global ref to keep the object alive until we decide it may
        // die, but delete the local ref because they are limited to 512 in
        // android and we should not trash them
        jobject local = *target;
        *target = env->NewGlobalRef(*target);
        env->DeleteLocalRef(local);
      }
    }

    virtual void* clone(void* obj)
    {
      jobject* ginstance = (jobject*)obj;

      if (!obj)
        return 0;

      jobject* cloned = new jobject;
      *cloned = *ginstance;

      qi::jni::JNIAttach attach;
      JNIEnv *env = attach.get();
      *cloned = env->NewGlobalRef(*cloned);

      return cloned;
    }

    virtual void destroy(void* obj)
    {
      if (!obj)
        return;
      // void* obj is a jobject
      jobject* jobj = (jobject*) obj;

      if (*jobj)
      {
        qi::jni::JNIAttach attach;
        JNIEnv *env = attach.get();
        env->DeleteGlobalRef(*jobj);
      }
      delete jobj;
    }

    virtual bool less(void* a, void* b)
    {
      jobject* pa = (jobject*) ptrFromStorage(&a);
      jobject* pb = (jobject*) ptrFromStorage(&b);

      return *pa < *pb;
    }
};

/* Register jobject -> See the above comment for explanations */
QI_TYPE_REGISTER_CUSTOM(jobject, JObjectTypeInterface);

