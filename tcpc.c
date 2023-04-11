#include "tcpc.h"
#include "carrow.h"
#include "evloop.h"

#include <netdb.h>
#include <unistd.h>


// static struct tcpcc 
// _connect_continue(struct tcpcc *self, struct tcpc *conn, int fd, int no) {
//     /* After epoll(2) indicates writability, use getsockopt(2) to read the
//        SO_ERROR option at level SOL_SOCKET to determine whether connect()
//        completed successfully (SO_ERROR is zero) or unsuccessfully
//        (SO_ERROR is one of the usual error codes listed here,
//        explaining the reason for the failure).
//     */
//     int err;
//     int l = 4;
//     if (conn->status == TCSCONNECTING) {
//         if (getsockopt(conn->fd, SOL_SOCKET, SO_ERROR, &err, &l)) {
//             conn->status = TCSFAILED;
//             return REJECT(self, conn, "tcpc_arm(%d)", conn->fd);
//         }
//         if (err != 0) {
//             conn->status = TCSFAILED;
//             errno = err;
//             close(conn->fd);
//             carrow_dearm(conn->fd);
//             return REJECT(self, conn, "tcp connect");
//         }
//         /* Hooray, Connected! */
//         conn->status = TCSCONNECTED;
//     }
// 
//     /* Already, Connected */
//     socklen_t socksize = sizeof(conn->localaddr);
//     if (getsockname(conn->fd, &(conn->localaddr), &socksize)) {
//         return REJECT(self, conn, "getsockname");
//     }
// 
//     return self->next;
// }


// struct tcpcc
// connectA(struct tcpcc *self, struct tcpc *conn, int fd_, int op) {
//     // if (conn->status == TCSCONNECTING) {
//     //     return _connect_continue(self, conn);
//     // }
//     // int cfd;
//     // struct addrinfo hints;
//     // struct addrinfo *result;
//     // struct addrinfo *try;
// 
//     // /* Resolve hostname */
//     // memset(&hints, 0, sizeof(hints));
//     // hints.ai_family = AF_INET;         /* Allow IPv4 only.*/
//     // // hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
//     // hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
//     // hints.ai_flags = AI_NUMERICSERV;
//     // hints.ai_protocol = 0;
//     // if (getaddrinfo(conn->host, conn->port, &hints, &result) != 0) {
//     //     return REJECT(self, conn, fd_, "Name resolution failed: %s", 
//     //             conn->host);
//     // }
// 
//     // /* getaddrinfo() returns a list of address structures.
//     //    Try each address until we successfully connect(2).
//     //    If socket(2) (or connect(2)) fails, we (close the socket
//     //    and) try the next address.
//     // */
//     // for (try = result; try != NULL; try = try->ai_next) {
//     //     cfd = socket(
//     //             try->ai_family,
//     //             try->ai_socktype | SOCK_NONBLOCK,
//     //             try->ai_protocol
//     //         );
//     //     if (cfd < 0) {
//     //         continue;
//     //     }
// 
//     //     if (connect(cfd, try->ai_addr, try->ai_addrlen) == 0) {
//     //         /* Connection success */
//     //         break;
//     //     }
// 
//     //     if (errno == EINPROGRESS) {
//     //         /* Waiting to connect */
//     //         break;
//     //     }
// 
//     //     close(cfd);
//     //     cfd = -1;
//     // }
// 
//     // if (cfd < 0) {
//     //     freeaddrinfo(result);
//     //     return REJECT(self, conn, fd_, "TCP connect");
//     // }
// 
//     // /* Connection success */
//     // /* Update state */
//     // memcpy(&(conn->remoteaddr), try->ai_addr, sizeof(struct sockaddr));
//     // 
//     // conn->fd = cfd;
//     // freeaddrinfo(result);
// 
//     // if (errno == EINPROGRESS) {
//     //     /* Waiting to connect */
//     //     /* The socket is nonblocking and the connection cannot be
//     //        completed immediately. It is possible to epoll(2) for
//     //        completion by selecting the socket for writing.
//     //     */
//     //     errno = 0;
//     //     conn->status = TCSCONNECTING;
// 
//     //     if (tcpc_arm(self, conn, conn->fd, EVOUT)) {
//     //         return REJECT(self, conn, conn->fd, "tcpc_arm");
//     //     }
//     // }
// 
//     // /* Seems everything is ok. */
//     // errno = 0;
//     // conn->status = TCSCONNECTED;
//     // return _connect_continue(self, conn);
// }
