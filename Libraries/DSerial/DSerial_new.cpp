/** @file dserial.cpp
 *  @brief The DSerial library implementation
 *
 *  @author Dillon Lareau (dlareau)
 */

#include "Arduino.h"
#include "DSerial.h"
#include <string.h>
#include "pointerQueue.h"

/** @brief Reads a packet from the specified stream if one is available
 *
 *  in: stream
 *  out: packet, client, len, retval
 *
 */
int readPacket(Stream &s, char *buffer){

}

/** @brief Writes a packet to the specified stream.
 *
 *  in: steam, packet, client, len
 *  out: retval
 *
 */
int sendPacket(Stream &s, char *message){

}

/** @brief Creates a new DSerialMaster object
 *
 *  @param port The underlying stream object used for communication.

 *  @return A new initialized DSerialMaster object
 */
DSerialMaster::DSerialMaster(Stream &port):_stream(port){

}

/** @brief sends a data string to the specified client.
 *
 *  in: packet, client, len
 *  out: retval
 *
 */
int DSerialMaster::sendData(uint8_t client_id, char *data){

}

/** @brief Retrieve data if there is any to get
 *
 *  in: NONE
 *  out: packet, client, len, retval
 *
 */
int DSerialMaster::getData(char *buffer){

}

/** @brief runs a client search
 *
 *  in: NONE
 *  out: numClients
 *
 */
int DSerialMaster::identifyClients() {

}

/** @brief gets the client array and number of clients
 *
 *  If the clients argument is NULL, the number of clients will still be
 *  returned, but it will not attempt to populate the array.
 *
 *  @param clients  a pointer to memory of at least MAX_CLIENTS size to put
                      the the clients into
 *  @return The number of clients found
 */
int DSerialMaster::getClients(uint8_t *clients){

}

int DSerialMaster::doSerial(){

}

/** @brief Creates a new DSerialClient object
 *
 *  @param port The underlying stream object used for communication.

 *  @return A new initialized DSerialClient object
 */
DSerialClient::DSerialClient(Stream &port, uint8_t client_number):_stream(port){

}

/** @brief sends a data string to the master.
 *
 *  Internally, this function enqueues the data to be written when convenient.
 *  This function fails when there is not enough memory to enqueue the message
 *  or when the queue is full.
 *
 *  @param data The data to write to the given client
 *  @return A status code indicating success or failure
 */
int DSerialClient::sendData(char *data){

}

/** @brief Retrieve data if there is any to get
 *
 *  @param buffer A string to populate with the possible data
 *  @return A status code indicating whether data was retrieved
 */
int DSerialClient::getData(char *buffer){

}

int DSerialClient::doSerial(){

}