//AUTOGENERATED HEADER DO NOT EDIT
/*
** Author(s):
**  - Cedric GESTES <gestes@aldebaran-robotics.com>
**
** Copyright (C) 2010 Aldebaran Robotics
*/

#ifndef  AL_MESSAGING_CALL_FUNCTOR_HPP_
# define AL_MESSAGING_CALL_FUNCTOR_HPP_

#include <alcommon-ng/collections/variables_list.hpp>
#include <alcommon-ng/functor/functor.hpp>

namespace AL
{

  template <typename R>
  R callFunctor(Functor *f) {
    AL::Messaging::ArgumentList  args;
    AL::Messaging::VariableValue ret;

    f->call(args, ret);
    return ret.as<R>();
  }


  template <typename R, typename P0>
  R callFunctor(Functor *f, const P0 &p0) {
    AL::Messaging::ArgumentList  args;
    AL::Messaging::VariableValue ret;

    args.push_back(p0);
    f->call(args, ret);
    return ret.as<R>();
  }


  template <typename R, typename P0, typename P1>
  R callFunctor(Functor *f, const P0 &p0, const P1 &p1) {
    AL::Messaging::ArgumentList  args;
    AL::Messaging::VariableValue ret;

    args.push_back(p0); args.push_back(p1);
    f->call(args, ret);
    return ret.as<R>();
  }


  template <typename R, typename P0, typename P1, typename P2>
  R callFunctor(Functor *f, const P0 &p0, const P1 &p1, const P2 &p2) {
    AL::Messaging::ArgumentList  args;
    AL::Messaging::VariableValue ret;

    args.push_back(p0); args.push_back(p1); args.push_back(p2);
    f->call(args, ret);
    return ret.as<R>();
  }


  template <typename R, typename P0, typename P1, typename P2, typename P3>
  R callFunctor(Functor *f, const P0 &p0, const P1 &p1, const P2 &p2, const P3 &p3) {
    AL::Messaging::ArgumentList  args;
    AL::Messaging::VariableValue ret;

    args.push_back(p0); args.push_back(p1); args.push_back(p2); args.push_back(p3);
    f->call(args, ret);
    return ret.as<R>();
  }


  template <typename R, typename P0, typename P1, typename P2, typename P3, typename P4>
  R callFunctor(Functor *f, const P0 &p0, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4) {
    AL::Messaging::ArgumentList  args;
    AL::Messaging::VariableValue ret;

    args.push_back(p0); args.push_back(p1); args.push_back(p2); args.push_back(p3); args.push_back(p4);
    f->call(args, ret);
    return ret.as<R>();
  }


  template <typename R, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5>
  R callFunctor(Functor *f, const P0 &p0, const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4, const P5 &p5) {
    AL::Messaging::ArgumentList  args;
    AL::Messaging::VariableValue ret;

    args.push_back(p0); args.push_back(p1); args.push_back(p2); args.push_back(p3); args.push_back(p4); args.push_back(p5);
    f->call(args, ret);
    return ret.as<R>();
  }

}
#endif
