#ifndef NETWORK_PROGRAMMING_MESSAGE_HPP_
#define NETWORK_PROGRAMMING_MESSAGE_HPP_

#include <string>

const std::string optWELCOME = std::string("[L]Login     [R]Register    [Q]Quit");
const std::string optMAIN = std::string("[SU]Show User         [SF]Show File List  [SE]Send Your File List  [LS]ls\n") +
                            std::string("[R]Request File List  [U]Upload           [D]Download\n") +
                            std::string("[C]Chat               [L]Logout           [DA]Delete Account");

const std::string msgREGISTER = "REGISTER";
const std::string msgLOGIN = "LOGIN";
const std::string msgLOGOUT = "LOGOUT";
const std::string msgDELETEACCOUNT = "DELETEACCOUNT";

const std::string msgUPDATECONNECTINFO = "UPDATECONNECTINFO";
const std::string msgUPDATEFILELIST = "UPDATEFILELIST";

const std::string msgSHOWUSER = "SHOWUSER";
const std::string msgSHOWFILELIST = "SHOWFILELIST";
const std::string msgGETUSERCONN = "GETUSERCONN";
const std::string msgGETFILELIST = "GETFILELIST";

const std::string msgCHATREQUEST = "CHATREQUEST";
const std::string msgMESSAGE = "MESSAGE";

const std::string msgFILEINFOREQUEST = "FILEINFOREQUEST";

const std::string msgFILEREAD = "FILEREAD";
const std::string msgFILEWRITE = "FILEWRITE";

const std::string msgSUCCESS = "Success!";
const std::string msgFAIL = "Failed!";

const std::string msgCHECKCONNECT = "CHECKCONNECTION";
const std::string msgDISCONNECTED = "Connection terminated!";

#endif // NETWORK_PROGRAMMING_MESSAGE_HPP_
