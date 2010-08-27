#!/usr/bin/env python
##
## generator.py
## Login : <ctaf42@cgestes-de2>
## Started on  Fri Aug 27 11:46:16 2010 Cedric GESTES
## $Id$
##
## Author(s):
##  - Cedric GESTES <gestes@aldebaran-robotics.com>
##
## Copyright (C) 2010 Cedric GESTES
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
##

#TypeNameList : typename P0, typename P1, ...
#TypeList     : P0, P1, ...
#ArgList      : const P0 &p0, const P1 & p1, ...
#ParamList    : p0, p1, ..
#CallParamList: pParams.getParameters()[0].as<P0>(), ...
#count    : 6
#

def read_file(fname):
    header   = ""
    footer   = ""
    template = ""
    mode = 0
    with open(fname, "r") as f:
        for line in f:
            stripped = line.strip("\r\n\t ")
            if stripped == "//HEADER":
                mode = 1
                continue
            if stripped == "//FOOTER":
                mode = 2
                continue
            if mode == 0:
                header += line
            elif mode == 1:
                template += line
            elif mode == 2:
                footer += line
    return (header, template, footer)

def generate_list(tpl, count, sep = ", ", begin = False, end = False):
    result = ""
    if begin:
        result += ", "
    for i in range(count):
        result += tpl % { 'count' : i }
        if i != count - 1 or end:
            result += ", "
    return result

def generate_callparam_list(count):
    t = "params.args()[%(count)d].as<P%(count)d>()"
    return generate_list(t, count)

def generate_typename_list(count):
    t = "typename P%(count)d"
    return generate_list(t, count, end = True)

def generate_type_list(count):
    t = "P%(count)d"
    return generate_list(t, count, end = True)

def generate_param_list(count):
    t = "const P%(count)d &p%(count)d"
    return generate_list(t, count)

def generate_arg_list(count):
    t = "p%(count)d"
    return generate_list(t, count)

def generate_code(head, tpl, foot, count):
    result = head
    for i in range(count):
        result += tpl % { 'count'        : i,
                          'TypeNameList' : generate_typename_list(i),
                          'TypeList'     : generate_type_list(i),
                          'ParamList'    : generate_param_list(i),
                          'ArgList'      : generate_arg_list(i),
                          'CallParamList': generate_callparam_list(i),
                          }
    result += foot
    return result

def generate_file(src, dst):
    (head, tpl, foot) = read_file(src)
    code = generate_code(head, tpl, foot, 6)
    with open(dst, "w") as f:
        f.write(code)

if __name__ == "__main__":
    generate_file("memberfunctor.hxx.in"    , "memberfunctor.hxx")
    generate_file("voidmemberfunctor.hxx.in", "voidmemberfunctor.hxx")
    generate_file("makefunctor.hpp.in"      , "makefunctor.hpp")
