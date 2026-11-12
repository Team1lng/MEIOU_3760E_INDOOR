#ifndef __PRIV_IPV6_H__
#define __PRIV_IPV6_H__


#ifdef __cplusplus
extern "C" {
#endif

#define IPV6_ADDR_ANY		    0x0000U
#define IPV6_ADDR_UNICAST      	0x0001U	
#define IPV6_ADDR_MULTICAST    	0x0002U	
#define IPV6_ADDR_LOOPBACK	    0x0010U
#define IPV6_ADDR_LINKLOCAL	    0x0020U
#define IPV6_ADDR_SITELOCAL	    0x0040U
#define IPV6_ADDR_COMPATv4	    0x0080U
#define IPV6_ADDR_SCOPE_MASK	0x00f0U
#define IPV6_ADDR_MAPPED	    0x1000U

#define IPV6_ADDR_MC_SCOPE(a)	((a)->s6_addr[1] & 0x0f)	/* nonstandard */
#define __IPV6_ADDR_SCOPE_INVALID	-1
#define IPV6_ADDR_SCOPE_NODELOCAL	0x01
#define IPV6_ADDR_SCOPE_LINKLOCAL	0x02
#define IPV6_ADDR_SCOPE_SITELOCAL	0x05
#define IPV6_ADDR_SCOPE_ORGLOCAL	0x08
#define IPV6_ADDR_SCOPE_GLOBAL		0x0e

#define IPV6_ADDR_SCOPE_TYPE(scope) ((scope) << 16)


#ifdef __cplusplus
}
#endif

#endif
