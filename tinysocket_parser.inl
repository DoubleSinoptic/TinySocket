#define NS_INT16SZ	 2
#define NS_INADDRSZ	 4
#define NS_IN6ADDRSZ	16

static int
ts_inet_pton4 (const char *src, u_char *dst)
{
        int saw_digit, octets, ch;
        u_char tmp[NS_INADDRSZ], *tp;

        saw_digit = 0;
        octets = 0;
        *(tp = tmp) = 0;
        while ((ch = *src++) != '\0') {

                if (ch >= '0' && ch <= '9') {
                        u_int news = *tp * 10 + (ch - '0');

                        if (saw_digit && *tp == 0)
                                return (0);
                        if (news > 255)
                                return (0);
                        *tp = news;
                        if (! saw_digit) {
                                if (++octets > 4)
                                        return (0);
                                saw_digit = 1;
                        }
                } else if (ch == '.' && saw_digit) {
                        if (octets == 4)
                                return (0);
                        *++tp = 0;
                        saw_digit = 0;
                } else
                        return (0);
        }
        if (octets < 4)
                return (0);
        memcpy(dst, tmp, NS_INADDRSZ);
        return (1);
}

static int
ts_inet_pton6 (const char *src, u_char *dst)
{
        static const char xdigits[] = "0123456789abcdef";
        u_char tmp[NS_IN6ADDRSZ], *tp, *endp, *colonp;
        const char *curtok;
        int ch, saw_xdigit;
        u_int val;

        tp = (u_char*)memset(tmp, '\0', NS_IN6ADDRSZ);
        endp = tp + NS_IN6ADDRSZ;
        colonp = NULL;
        /* Leading :: requires some special handling. */
        if (*src == ':')
                if (*++src != ':')
                        return (0);
        curtok = src;
        saw_xdigit = 0;
        val = 0;
        while ((ch = tolower (*src++)) != '\0') {
                const char *pch;

                pch = strchr(xdigits, ch);
                if (pch != NULL) {
                        val <<= 4;
                        val |= (pch - xdigits);
                        if (val > 0xffff)
                                return (0);
                        saw_xdigit = 1;
                        continue;
                }
                if (ch == ':') {
                        curtok = src;
                        if (!saw_xdigit) {
                                if (colonp)
                                        return (0);
                                colonp = tp;
                                continue;
                        } else if (*src == '\0') {
                                return (0);
                        }
                        if (tp + NS_INT16SZ > endp)
                                return (0);
                        *tp++ = (u_char) (val >> 8) & 0xff;
                        *tp++ = (u_char) val & 0xff;
                        saw_xdigit = 0;
                        val = 0;
                        continue;
                }
                if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
                    ts_inet_pton4(curtok, tp) > 0) {
                        tp += NS_INADDRSZ;
                        saw_xdigit = 0;
                        break;	/* '\0' was seen by inet_pton4(). */
                }
                return (0);
        }
        if (saw_xdigit) {
                if (tp + NS_INT16SZ > endp)
                        return (0);
                *tp++ = (u_char) (val >> 8) & 0xff;
                *tp++ = (u_char) val & 0xff;
        }
        if (colonp != NULL) {
                /*
                 * Since some memmove()'s erroneously fail to handle
                 * overlapping regions, we'll do the shift by hand.
                 */
                const int n = tp - colonp;
                int i;

                if (tp == endp)
                        return (0);
                for (i = 1; i <= n; i++) {
                        endp[- i] = colonp[n - i];
                        colonp[n - i] = 0;
                }
                tp = endp;
        }
        if (tp != endp)
                return (0);
        memcpy(dst, tmp, NS_IN6ADDRSZ);
        return (1);
}

static const char *
ts_inet_ntop4 (const u_char *src, char *dst, socklen_t size)
{
        static const char fmt[] = "%u.%u.%u.%u";
        char tmp[sizeof "255.255.255.255"];

        if (sprintf(tmp, fmt, src[0], src[1], src[2], src[3]) >= size) {
                throw ts::socket_exception("error: address string overflow");
                return (NULL);
        }
        return strcpy(dst, tmp);
}

static const char *
ts_inet_ntop6 (const u_char *src, char *dst, size_t size)
{
        /*
         * Note that int32_t and int16_t need only be "at least" large enough
         * to contain a value of the specified size.  On some systems, like
         * Crays, there is no such thing as an integer variable with 16 bits.
         * Keep this in mind if you think this function should have been coded
         * to use pointer overlays.  All the world's not a VAX.
         */
        char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
        struct { int base, len; } best, cur;
        u_int words[NS_IN6ADDRSZ / NS_INT16SZ];
        int i;

        /*
         * Preprocess:
         *	Copy the input (bytewise) array into a wordwise array.
         *	Find the longest run of 0x00's in src[] for :: shorthanding.
         */
        memset(words, '\0', sizeof words);
        for (i = 0; i < NS_IN6ADDRSZ; i += 2)
                words[i / 2] = (src[i] << 8) | src[i + 1];
        best.base = -1;
        cur.base = -1;
        best.len = 0;
        cur.len = 0;
        for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
                if (words[i] == 0) {
                        if (cur.base == -1)
                                cur.base = i, cur.len = 1;
                        else
                                cur.len++;
                } else {
                        if (cur.base != -1) {
                                if (best.base == -1 || cur.len > best.len)
                                        best = cur;
                                cur.base = -1;
                        }
                }
        }
        if (cur.base != -1) {
                if (best.base == -1 || cur.len > best.len)
                        best = cur;
        }
        if (best.base != -1 && best.len < 2)
                best.base = -1;

        /*
         * Format the result.
         */
        tp = tmp;
        for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++) {
                /* Are we inside the best run of 0x00's? */
                if (best.base != -1 && i >= best.base &&
                    i < (best.base + best.len)) {
                        if (i == best.base)
                                *tp++ = ':';
                        continue;
                }
                /* Are we following an initial run of 0x00s or any real hex? */
                if (i != 0)
                        *tp++ = ':';
                /* Is this address an encapsulated IPv4? */
                if (i == 6 && best.base == 0 &&
                    (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
                        if (!ts_inet_ntop4(src+12, tp, sizeof tmp - (tp - tmp)))
                                return (NULL);
                        tp += strlen(tp);
                        break;
                }
                tp += sprintf(tp, "%x", words[i]);
        }
        /* Was it a trailing run of 0x00's? */
        if (best.base != -1 && (best.base + best.len) ==
            (NS_IN6ADDRSZ / NS_INT16SZ))
                *tp++ = ':';
        *tp++ = '\0';

        /*
         * Check for overflow, copy, and we're done.
         */
        if ((socklen_t)(tp - tmp) > size) {
                throw ts::socket_exception("error: address string overflow");
                return (NULL);
        }
        return strcpy(dst, tmp);
}

int
ts_inet_pton (int af, const char *src, void *dst)
{
        switch (af) {
        case AF_INET:
                return (ts_inet_pton4(src, (u_char*)dst));
        case AF_INET6:
                return (ts_inet_pton6(src, (u_char*)dst));
        default:
                throw ts::socket_exception("error: address famaly not supported");
                return (-1);
        }
        /* NOTREACHED */
}

const char *
ts_inet_ntop (int af, const void *src, char *dst, socklen_t size)
{
        switch (af) {
        case AF_INET:
                return (const char *)(ts_inet_ntop4((const u_char*)src, dst, size));
        case AF_INET6:
                return (const char *)(ts_inet_ntop6((const u_char*)src, dst, size));
        default:
			    throw ts::socket_exception("error: address famaly not supported");
                return (NULL);
        }
        /* NOTREACHED */
}