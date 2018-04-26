#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

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

  void Echo(const std::string& user) {
    HelloRequest request;
    request.set_name(user);
    HelloReply reply;
    ClientContext context;
    Status status = stub_->HelloReq(&context, request, &reply);
    if (status.ok()) {
      // std::cout<< "Received " << reply.message() << std::endl;
    } else {
      std::cout<< status.error_code() << " : "
               << status.error_message() << std::endl;
      return;
    }
  }


 private:
  std::unique_ptr<Greeter::Stub> stub_;

};

int main() {
  HelloClient greeter(
      grpc::CreateChannel("localhost:50001", grpc::InsecureChannelCredentials()));

  std::string user("async world!");
  for (int i = 0; i < 1000000; i++) {
    greeter.Echo(user);
  }
  return 0;
}
