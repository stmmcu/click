/*
 * packetshaper2.{cc,hh} -- element limits number of successful pulls per
 * second to a given rate (packets/s). unlike packetshaper, does not use EWMA.
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology.
 *
 * This software is being provided by the copyright holders under the GNU
 * General Public License, either version 2 or, at your discretion, any later
 * version. For more information, see the `COPYRIGHT' file in the source
 * distribution.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "packetshaper2.hh"
#include "confparse.hh"
#include "error.hh"
#include "glue.hh"

PacketShaper2::PacketShaper2()
{
  add_input();
  add_output();
}

PacketShaper2::~PacketShaper2()
{
}

PacketShaper2 *
PacketShaper2::clone() const
{
  return new PacketShaper2;
}

int
PacketShaper2::configure(const Vector<String> &conf, ErrorHandler *errh)
{
  int rate;
  if (cp_va_parse(conf, this, errh,
		  cpUnsigned, "max allowable rate", &rate,
		  0) < 0)
    return -1;

  _meter = rate;
  _ugap = 1000000/rate;
  _total = 0;
  _start.tv_sec = 0;
  _start.tv_usec = 0;
  return 0;
}

Packet *
PacketShaper2::pull(int)
{
  struct timeval now;
  click_gettimeofday(&now);

  if (_start.tv_sec == 0) _start = now;
  else {
    struct timeval diff;
    timersub(&now, &_start, &diff);
    
    unsigned need = diff.tv_sec * _meter;
    need += diff.tv_usec / _ugap;

    if (need > _total) {
      _total++;
      Packet *p = input(0).pull();
      return p;
    }

    if (_total > _meter * 360) {
      _total = 0;
      _start = now;
    }
  }
  return 0;
}

EXPORT_ELEMENT(PacketShaper2)
