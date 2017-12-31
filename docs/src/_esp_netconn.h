/**
 * \addtogroup      ESP_NETCONN
 * \{
 *
 * Netconn provides sequential API to work with connection 
 * in either server or client mode.
 *
 * Netconn API can handle reading asynchronous network data in synchronous way
 * by using operating system features such as message queues 
 * and by putting thread into blocking mode, it allows zero overhead 
 * from performance point of view.
 * 
 * \section         sect_netconn_client Netconn client
 *
 * \image html netconn_client.svg Netconn API architecture
 *
 * Every netconn structure consists of at least data message queue 
 * to handle received packets before user reads them in user thread.
 *
 * On image we can see blue box showing connection handle data queue.
 * Data queue is filled from connection callback which is dedicated specially for
 * netconn type connections.
 *
 * When user wants to read data from connection,
 * thread will be blocked until something is available in received data message queue.
 *
 * To allow user to handle closed connections while waiting for more data,
 * information about closed connection is also added to received data message queue.
 *
 * \par             Example
 * 
 * Example shows how to use netconn API to write and read data in synchronous way,
 * no need to have complex code structure for asynchronous data reception callbacks
 *
 * \code{c}
/* Set server parameters */
#define NETCONN_HOST            "example.com"
#define NETCONN_PORT            80

/* Request data for netconn */
const uint8_t netconn_req_data[] = ""
    "GET / HTTP/1.1\r\n"
    "Host: " NETCONN_HOST "\r\n"
    "Connection: close\r\n"
    "\r\n";

/*
 * \brief           Client netconn thread
 */
void
client_thread(void const* arg) {
    espr_t res;
    esp_pbuf_p pbuf;
    esp_netconn_p client;
                                                
    /*
     * First create a new instance of netconn 
     * connection and initialize system message boxes
     * to accept clients and packet buffers
     */
    client = esp_netconn_new(ESP_NETCONN_TYPE_TCP);
    
    if (client != NULL) {                       /* Process only if client memory is allocated */
        
        /*
         * Connect to external server as client
         * with custom NETCONN_CONN_HOST and CONN_PORT values
         *
         * Function will block thread until we are successfully connected to server
         */
        res = esp_netconn_connect(client, NETCONN_HOST, NETCONN_PORT);
        if (res == espOK) {                     /* Are we successfully connected? */
            printf("Connected to server " NETCONN_HOST "\r\n");
            res = esp_netconn_write(client, requestData, sizeof(requestData) - 1);    /* Send data to server */
            if (res == espOK) {                 /* Were data sent? */
                printf("Data were successfully sent to server\r\n");
                
                /*
                 * Since we sent HTTP request, 
                 * we are expecting some data from server
                 * or at least forced connection close from remote side
                 */
                do {
                    
                    /*
                     * Receive single packet of data
                     * 
                     * Function will block thread until new packet 
                     * is ready to be read from remote side
                     *
                     * After function returns, check status as it
                     * may happen that closed status was returned
                     */
                    res = esp_netconn_receive(client, &pbuf);
                    if (res == espCLOSED) {     /* Was the connection closed? This can be checked by return status of receive function */
                        printf("Connection closed by remote side...Stopping\r\n");
                        break;
                    }
                    
                    /*
                     * At this point read and manipulate
                     * with received buffer and check if you expect more data
                     *
                     * After you are done using it, it is important
                     * you free the memory otherwise memory leaks will appear
                     */
                    printf("Received new data packet of %d bytes\r\n", (int)esp_pbuf_length(pbuf, 1));
                    esp_pbuf_free(pbuf);        /* Free the memory after usage */
                } while (1);
            } else {
                printf("Error writing data to remote host!\r\n");
            }
            
            /*
             * Check if connection was closed by remote server
             * and in case it wasn't, close it by yourself
             */
            if (res != espCLOSED) {
                esp_netconn_close(client);
            }
        } else {
            printf("Cannot connect to external server!\r\n");
        }
    }
    
    /*
     * Last step is to delete connection object from memory
     */
    esp_netconn_delete(client);
}
\endcode
 *
 * \}
 */

/**
 * \addtogroup      ESP_NETCONN
 * \{
 *
 * \section         sect_netconn_server Netconn server
 *
 * Netconn API allows implementation of server in similar way like client mode.
 *
 * In addition to client, some additional steps must be included:
 *
 *  - Connection must be set to listening mode
 *  - Connection must wait and accept new client
 *
 * \image html netconn_server.svg Server mode netconn architecture
 *
 * If netconn API is used as server mode, accept message queue is introduced.
 * This message queue handles every new connection which is active on
 * dedicated port for server mode we are listening on.
 *
 * When a new client connects, new fresh client structure is created,
 * and put to server's accept message queue. This structure is later used
 * to write received data to it, so when user accepts a connection,
 * it may already have some data to read immediately.
 *
 * Once new client is received with \ref esp_netconn_accept function,
 * control is given to client object which can be later
 * read and written in the same way as client mode.
 * 
 * \par             Example
 *
 * \code{c}
/*
 * \brief           Server netconn thread
 */
void
server_thread(void const* arg) {
    espr_t res;
    esp_pbuf_p pbuf;
    esp_netconn_p server, client;
                                                
    /*
     * First create a new instance of netconn 
     * connection and initialize system message boxes
     * to accept clients and packet buffers
     */
    server = esp_netconn_new(ESP_NETCONN_TYPE_TCP);
    
    if (server != NULL) {
        
        /*
         * Bind network connection to port 80
         */
        res = esp_netconn_bind(server, 80);
        if (res == espOK) {
            
            /*
             * Start listening for incoming connections
             * on previously binded port
             */
            res = esp_netconn_listen(server);
            
            /*
             * Loop forever
             */
            while (1) {
                
                /*
                 * Wait and accept new client connection
                 * 
                 * Function will block thread until 
                 * new client is connected to server.
                 *
                 * From performance point of view,
                 * this allows you zero overhead
                 */
                res = esp_netconn_accept(server, &client);
                if (res == espOK) {
                    printf("A new connection accepted!\r\n");
                    
                    do {
                        /*
                         * Since we accepted a client, we
                         * are expecting that client will send to us some data
                         *
                         * Wait for data and block thread for that time
                         */
                        res = esp_netconn_receive(client, &pbuf);
                        
                        /*
                         * It may happen that connection 
                         * is closed from client side
                         */
                        if (res == espCLOSED) {
                            break;
                        }
                        
                        /*
                         * Process buffer and decide if you expect more data,
                         * such as CRLFCRLF sequence in HTTP server.
                         *
                         * When you are ready to continue, break this for loop
                         */
                        esp_pbuf_free(pbuf);    /* Do not forget to free memory after usage! */
                        
                        /*
                        if (everything_received(client)) {
                            break;
                        }
                        */
                    } while (res == espOK);
                    
                    /*
                     * If everything is still ready,
                     * it is time to send response to client
                     */
                    if (res == espOK) {
                        
                        /*
                         * Send data back to client
                         * and wait to be sent successfully
                         *
                         * Data are written to TCP send buffer
                         * to allow user to call write function multiple 
                         * times and to speed up communication
                         */
                        res = esp_netconn_write(client, your_data, your_data_len);
                        
                        /*
                         * When done sending everything, close client connection
                         */
                        esp_netconn_close(client);
                    }
                    
                    /*
                     * Last step is to free memory 
                     * for netconn and to go to ready state for next connection
                     */
                    esp_netconn_delete(client);
                }
            }
        }
    }
}
\endcode
 *
 * \}
 */