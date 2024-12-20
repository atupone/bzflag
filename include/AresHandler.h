/* bzflag
 * Copyright (c) 1993-2023 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef __ARES_HANDLER_H__
#define __ARES_HANDLER_H__

// bzflag global header
#include "global.h"

/* common implementation headers */
#include "network.h"
#include <ares.h>

class AresHandler
{
public:
    AresHandler(int index);
    ~AresHandler();

    static bool   globalInit();
    static void   globalShutdown();

    enum ResolutionStatus
    {
        None = 0,
        Failed,
        HbAPending,
        HbASucceeded,
        HbNPending,
        HbNSucceeded
    };

    void      setIndex ( int i )
    {
        index = i;
    }
    void      queryHostname(const struct sockaddr *clientAddr);
    void      queryHost(const char *hostName);
    const char   *getHostname();
    ResolutionStatus getHostAddress(struct in_addr *clientAddr);
    void      setFd(fd_set *read_set, fd_set *write_set, int &maxFile);
    void      process(fd_set *read_set, fd_set *write_set);
    ResolutionStatus getStatus()
    {
        return status;
    };
private:
#if HAVE_ARES_GETADDRINFO
    static void staticCallbackAddrInfo(void *arg, int status,
                                       int timeout, struct ares_addrinfo *result);
    void      callbackAddrInfo(int status, struct ares_addrinfo *result);
#endif
    static void   staticCallback(void *arg, int statusCallback, int timeouts,
                                 struct hostent *hostent);
    void      callback(int status, struct hostent *hostent);

    int       index;

    std::string   hostName;
    in_addr   hostAddress;
    ares_channel  aresChannel;
    ResolutionStatus status;
    bool      aresFailed;

    static bool   globallyInited;

};

#endif

// Local Variables: ***
// mode: C++ ***
// tab-width: 4 ***
// c-basic-offset: 4 ***
// indent-tabs-mode: nil ***
// End: ***
// ex: shiftwidth=4 tabstop=4
