/* SPDX-License-Identifier: LGPL-2.1-or-later
 * Copyright © 2019 VMware, Inc. */
#pragma once

#include "conf-parser.h"
#include "networkd-util.h"

typedef struct Link Link;
typedef struct Manager Manager;
typedef struct Network Network;
typedef struct Request Request;

typedef enum TClassKind {
        TCLASS_KIND_DRR,
        TCLASS_KIND_HTB,
        TCLASS_KIND_QFQ,
        _TCLASS_KIND_MAX,
        _TCLASS_KIND_INVALID = -EINVAL,
} TClassKind;

typedef struct TClass {
        Link *link;
        Network *network;
        ConfigSection *section;
        NetworkConfigSource source;
        NetworkConfigState state;

        uint32_t classid;
        uint32_t parent;

        TClassKind kind;
        char *tca_kind;
} TClass;

typedef struct TClassVTable {
        size_t object_size;
        const char *tca_kind;
        /* called in tclass_new() */
        int (*init)(TClass *tclass);
        int (*fill_message)(Link *link, TClass *tclass, sd_netlink_message *m);
        int (*verify)(TClass *tclass);
} TClassVTable;

extern const TClassVTable * const tclass_vtable[_TCLASS_KIND_MAX];

#define TCLASS_VTABLE(t) ((t)->kind != _TCLASS_KIND_INVALID ? tclass_vtable[(t)->kind] : NULL)

/* For casting a tclass into the various tclass kinds */
#define DEFINE_TCLASS_CAST(UPPERCASE, MixedCase)                          \
        static inline MixedCase* TCLASS_TO_##UPPERCASE(TClass *t) {       \
                if (_unlikely_(!t || t->kind != TCLASS_KIND_##UPPERCASE)) \
                        return NULL;                                      \
                                                                          \
                return (MixedCase*) t;                                    \
        }

DEFINE_NETWORK_CONFIG_STATE_FUNCTIONS(TClass, tclass);

TClass* tclass_free(TClass *tclass);
int tclass_new_static(TClassKind kind, Network *network, const char *filename, unsigned section_line, TClass **ret);

void tclass_hash_func(const TClass *qdisc, struct siphash *state);
int tclass_compare_func(const TClass *a, const TClass *b);

int link_find_tclass(Link *link, uint32_t classid, TClass **ret);

int request_process_tclass(Request *req);
int link_request_tclass(Link *link, TClass *tclass);

void network_drop_invalid_tclass(Network *network);

int manager_rtnl_process_tclass(sd_netlink *rtnl, sd_netlink_message *message, Manager *m);

DEFINE_SECTION_CLEANUP_FUNCTIONS(TClass, tclass_free);

CONFIG_PARSER_PROTOTYPE(config_parse_tclass_parent);
CONFIG_PARSER_PROTOTYPE(config_parse_tclass_classid);

#include "drr.h"
#include "htb.h"
#include "qfq.h"
