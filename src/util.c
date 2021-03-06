#include "spnuv.h"

int spnuv_parse_addr(char *host, int port, struct sockaddr_storage *ss) {
        struct in_addr addr4;
        struct in6_addr addr6;
        struct sockaddr_in *sa4;
        struct sockaddr_in6 *sa6;
        unsigned int scope_id;
        unsigned int flowinfo;

        flowinfo = scope_id = 0;

        memset(ss, 0, sizeof(struct sockaddr_storage));

        if (host[0] == '\0') {
                sa4 = (struct sockaddr_in *) ss;
                sa4->sin_family = AF_INET;
                sa4->sin_port = htons((short)port);
                sa4->sin_addr.s_addr = INADDR_ANY;
                return 0;
        } else if (host[0] == '<' && strcmp(host, "<broadcast>") == 0) {
                sa4 = (struct sockaddr_in *)ss;
                sa4->sin_family = AF_INET;
                sa4->sin_port = htons((short)port);
                sa4->sin_addr.s_addr = INADDR_BROADCAST;
                return 0;
        } else if (uv_inet_pton(AF_INET, host, &addr4) == 0) {
                /* it's an IPv4 address */
                sa4 = (struct sockaddr_in *)ss;
                sa4->sin_family = AF_INET;
                sa4->sin_port = htons((short)port);
                sa4->sin_addr = addr4;
                return 0;
        } else if (uv_inet_pton(AF_INET6, host, &addr6) == 0) {
                /* it's an IPv6 address */
                sa6 = (struct sockaddr_in6 *)ss;
                sa6->sin6_family = AF_INET6;
                sa6->sin6_port = htons((short)port);
                sa6->sin6_addr = addr6;
                sa6->sin6_flowinfo = flowinfo;
                sa6->sin6_scope_id = scope_id;
                return 0;
        } else {
                return -1;
        }

        return 0;
}

SpnValue spnuv_get_error(int code, const char *name, const char *message) {
        SpnHashMap *error = spn_hashmap_new();
        SpnValue ret;
        SpnValue value;

        value = spn_makeint(code);
        spn_hashmap_set_strkey(error, "code", &value);
        spn_value_release(&value);

        value = spn_makestring(name);
        spn_hashmap_set_strkey(error, "name", &value);
        spn_value_release(&value);

        value = spn_makestring(message);
        spn_hashmap_set_strkey(error, "message", &value);
        spn_value_release(&value);

        ret.type = SPN_TYPE_OBJECT;
        ret.v.o = error;

        return ret;
}
