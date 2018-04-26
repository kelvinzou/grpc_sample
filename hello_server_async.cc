#include <memory>
#include <iostream>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include "hello.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

class HelloServer final {
 public:
  ~HelloServer() {
    server_->Shutdown();
    // Always shutdown the completion queue after the server, since otherwise
    // we may have an undrained queue.
    completion_q_->Shutdown();
  }

  void Run() {
    // Create the server side listening to service.
    std::string server_addr("localhost:50001");

    // Add a builder to the server.
    ServerBuilder builder;
    // Listening locally on a specific port.
    builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
    // Register the async service.
    builder.RegisterService(&service_);
    // Add the completion queue to the system since it is Async model.
    completion_q_ = builder.AddCompletionQueue();
    // Finally assemble server and start.
    server_ = builder.BuildAndStart();
    // The main handle rpc loop.
    HandleRpcs();
  }

 private:
  // The class that wraps each individual request and reply.
  class CallData {
   public:
    CallData(Greeter::AsyncService* service, ServerCompletionQueue* cq)
        : service_(service), cq_(cq), responder_(&ctx_) {
      // As part of the initial ctor, we request the system to start processing
      // the request and in this request, "this" acts are the tag uniquely
      // identifying the request (so that different class CallData() can serve
      // different requests concurrently, in this case it is the memory addr of
      // this CallData instance.
      state_ = PROCESS;
      service_->RequestHelloReq(&ctx_, &req_, &responder_, cq_, cq_, this);
    }

    void Proceed() {
      if (state_ == PROCESS) {
        // Open a new instance for listening, and the instance will deallocate itself
        // as part of the FINISH state.
        new CallData(service_, cq_);

        // This is for response.
        std::string prefix("Hello: ");
        reply_.set_message(prefix + req_.name()+ " Process peer is: " + ctx_.peer());
        // At this point we are done, we let the class know that we are finished
        // and rely on the status_ to let the class know that we are finished.
        state_ = FINISH;
        responder_.Finish(reply_, Status::OK, this);
      } else {
        GPR_ASSERT(state_ == FINISH);

        // Other actions may be added before calling delete this.

        // Once it is in finish state, delete ourself.
        delete this;
      }
    }

   private:
    // The means of communicate with ProceedGRPC runtime for async server.
    Greeter::AsyncService* service_;
    // Producer-consumer queue for Async notification.
    ServerCompletionQueue* cq_;
    // Context for the rpc.
    ServerContext ctx_;

    // Request and Response, cached for acking the response.
    HelloRequest req_;
    HelloReply reply_;

    // Responder, the means to get back to the client.
    ServerAsyncResponseWriter<HelloReply> responder_;

    // A simple state machine.
    enum CallState {PROCESS, FINISH};
    CallState state_; // the current Serving state.
  };

  void HandleRpcs() {
    // Spawn a the first CallCallData instance to serve a new client.
    new CallData(&service_, completion_q_.get());
    // This is used as a pointer to point to the data.
    void* tag;
    bool ok;
    while(true) {
      // Block waiting to read the *Next()* event from the completion
      // queue. The event is uniquely identified by its tag, and in
      // this case it is the memory address of the callback CallCallData
      // instance.
      // The return value of Next should always be checked. This return
      // value tells us whether there is any kind of event or cq_ is
      // shutting down.
      GPR_ASSERT(completion_q_->Next(&tag, &ok));
      GPR_ASSERT(ok);
      static_cast<CallData*>(tag)->Proceed();
    }
  }
  // This is a Async Service, used for grpc registration.
  Greeter::AsyncService service_;
  // The completion queue to track active requests.
  std::unique_ptr<ServerCompletionQueue> completion_q_;
  // This is the server pointer that is used to track the server that
  // serves the grpc server request.
  std::unique_ptr<Server> server_;
};


int main() {
  HelloServer server;
  server.Run();
  return 0;
}
