//
//  Hello World server
//  Binds REP socket to tcp://*:5555
//  Expects "Hello" from client, replies with "World"
//

// TODO: Windows.h tries to redefine (some or more?) structs exported
// by time.h (and possibly others). Things seem to work if windows.h
// is included first. I wonder if that breaks anything...
#include <windows.h> //rename("timeval", "wtimeval")

#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <zmq.h>

// C++ headers
#include <iostream>

#include "NxCoreCmdProto.pb.h"

void testLoadDLL();
void testCC();
void testZMQServer(char* fqn);

//int WINAPI WinMain(HINSTANCE d1, HINSTANCE d2, LPSTR d3, int d4)
int main(int argc, char** argv) {

  // This is just to show that we can indeed call Win32 functions
  MessageBox(NULL, "Hello, World!", "", MB_OK);

  // Test loading a dll and getting a proc address
  testLoadDLL ();

  // g++ -fPIC -c src/gen/c/NxCoreConfigProto.pb.cc

  // Try instantiating a C++ struct
  testCC ();

  // Test a linux library
  testZMQServer ("ipc:///tmp/feeds/0");

  return 0;
}

void testLoadDLL()
{
  HMODULE hLib = LoadLibrary("zlibwapi.dll");

  if(hLib == NULL) {
    // See http://msdn.microsoft.com/en-us/library/aa450919.aspx
    // for a list of system error codes
    // 193 = ERROR_BAD_EXE_FORMAT
    std::cout << "Failed loading zlib1.dll. Error " << GetLastError() << "\n";
    return;
  }

  // TODO: In a perfect world we know about the type signature of the
  // function and would thus use typedef to add at least some
  // typesafety, something like 'typedef UINT (CALLBACK*
  // LPFNDLLFUNC1)(DWORD,UINT);'
  FARPROC func = GetProcAddress(hLib,"inflate");
  if(func == 0) {
    printf ("GetProcAddress failed...\n");
  }
}

void testCC()
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  NxCoreConfigProto::NxCoreConfigEvent event;

  google::protobuf::ShutdownProtobufLibrary();
}

void testZMQServer(char* fqn)
{
  // See http://www.parashift.com/c++-faq-lite/mixing-c-and-cpp.html
  // on some usage hint on void pointers (bottom of page).  In general
  // void* is used as a form of polymorphism when the target type is
  // unknown.
  void *context = zmq_init (1);

  // Socket to talk to client
  void *responder = zmq_socket (context, ZMQ_REP);
  int rc = zmq_bind (responder, fqn);

  // Check that bind was successful
  //assert (rc == 0);
  if(rc != 0) {
    printf ("ZMQ error %s", zmq_strerror(rc));
  }

  while (1) {
    // Wait for next request from client
    zmq_msg_t request;
    zmq_msg_init (&request);
    zmq_recv (responder, &request, 0);

    // would work as well
    std::cout << "Received Hello (c++), ";
    printf ("Received Hello (c)\n");

    zmq_msg_close (&request);

    //  Do some 'work'
    Sleep (1000); // This is Windows sleep function
    sleep (1);

    //  Send reply back to client
    zmq_msg_t reply;
    zmq_msg_init_size (&reply, 5);
    memcpy (zmq_msg_data (&reply), "World", 5);
    zmq_send (responder, &reply, 0);
    zmq_msg_close (&reply);

  }
  //  We never get here but if we did, this would be how we end
  zmq_close (responder);
  zmq_term (context);

}
