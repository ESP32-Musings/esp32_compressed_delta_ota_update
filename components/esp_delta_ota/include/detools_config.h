/*
 * libcoap configure implementation for ESP32 platform.
 *
 * Uses libcoap software implementation for failover when concurrent
 * configure operations are in use.
 *
 * coap.h -- main header file for CoAP stack of libcoap
 *
 * Copyright (C) 2010-2012,2015-2016 Olaf Bergmann <bergmann@tzi.org>
 *               2015 Carsten Schoenert <c.schoenert@t-online.de>
 *
 * Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
 *
 * This file is part of the CoAP library libcoap. Please see README for terms
 * of use.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_


#ifdef CONFIG_DETOOLS_CONFIG_FILE_IO
#define DETOOLS_CONFIG_FILE_IO                 1
#else
#define DETOOLS_CONFIG_FILE_IO                 0
#endif

#ifdef CONFIG_DETOOLS_CONFIG_COMPRESSION_NONE
#define DETOOLS_CONFIG_COMPRESSION_NONE        1
#else
#define DETOOLS_CONFIG_COMPRESSION_NONE        0
#endif

#ifdef CONFIG_DETOOLS_CONFIG_COMPRESSION_LZMA
#define DETOOLS_CONFIG_COMPRESSION_LZMA        1
#else
#define DETOOLS_CONFIG_COMPRESSION_LZMA        0
#endif

#ifdef CONFIG_DETOOLS_CONFIG_COMPRESSION_CRLE
#define DETOOLS_CONFIG_COMPRESSION_CRLE        1
#else
#define DETOOLS_CONFIG_COMPRESSION_CRLE        0
#endif

#ifdef CONFIG_DETOOLS_CONFIG_COMPRESSION_HEATSHRINK
#define DETOOLS_CONFIG_COMPRESSION_HEATSHRINK  1
#else 
#define DETOOLS_CONFIG_COMPRESSION_HEATSHRINK  0
#endif


#endif /* _CONFIG_H_ */
