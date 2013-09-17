#ifndef CSA701_APP_SENDER_H
#define CSA701_APP_SENDER_H


extern void externalIocSenderStart(int socket_type,
                              int ioc_address_local,
                              int ioc_address_dest,
                              int ioc_port_dest,
                              int mode);
#endif /* CSA701_APP_SENDER_H */
