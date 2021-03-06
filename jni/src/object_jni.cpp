/*
**
** Author(s):
**  - Pierre ROULLON <proullon@aldebaran-robotics.com>
**
** Copyright (C) 2013 Aldebaran Robotics
** See COPYING for the license
*/

#include <stdexcept>
#include <sstream>

#include <qi/log.hpp>
#include <qi/type/dynamicobject.hpp>

#include <jnitools.hpp>
#include <object_jni.hpp>

qiLogCategory("qimessaging.jni");

JNIObject::JNIObject(const qi::AnyObject& o)
{
  // FIXME where is the delete?
  qi::AnyObject* newO = new qi::AnyObject();
  *newO = o;

  this->build(newO);
}

JNIObject::JNIObject(qi::AnyObject *newO)
{
  this->build(newO);
}

JNIObject::JNIObject(jobject value)
{
  _env = attach.get();

  _obj = value;
}

jobject JNIObject::object()
{
  return _obj;
}

qi::AnyObject      JNIObject::objectPtr()
{
  jfieldID fid = _env->GetFieldID(cls_anyobject, "_p", "J");

  if (!fid)
  {
    qiLogFatal() << "JNIObject: Cannot get GenericObject";
    throw "JNIObject: Cannot get GenericObject";
  }

  jlong fieldValue = _env->GetLongField(_obj, fid);
  return *(reinterpret_cast<qi::AnyObject*>(fieldValue));
}

void JNIObject::build(qi::AnyObject *newO)
{
  _env = attach.get();

  jmethodID mid = _env->GetMethodID(cls_anyobject, "<init>", "(J)V");

  if (!mid)
  {
    qiLogError() << "JNIObject : Cannot find constructor";
    throwNewException(_env, "JNIObject : Cannot find constructor");
  }

  jlong pObj = (long) newO;

  _obj = _env->NewObject(cls_anyobject, mid, pObj);

  // Keep a global ref on this object to avoid destruction EVER
  _env->NewGlobalRef(_obj);
}
