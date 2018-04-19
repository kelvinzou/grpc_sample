#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "hello.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

class HelloClient {
 public:
  HelloClient(std::shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)){}

  std::string Echo(const std::string& user) {
    HelloRequest request;
    request.set_name(user);

    HelloReply reply;
    ClientContext context;
    Status status = stub_->HelloReq(&context, request, &reply);
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout<< status.error_code() << " : "
               << status.error_message() << std::endl;
      return "RPC failed!";
    }
  }


 private:
  std::unique_ptr<Greeter::Stub> stub_;

};

int main() {
  HelloClient greeter(
      grpc::CreateChannel("localhost:50001", grpc::InsecureChannelCredentials()));

  std::string user("haha");
  std::string reply = greeter.Echo(user);
  std::cout<< "Received: " << reply << std::endl;
  return 0;
}
