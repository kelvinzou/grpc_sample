#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "hello.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using helloworld::HelloRequest;
using helloworld::HelloReply;
using helloworld::Greeter;

class HelloworldService final : public Greeter::Service {
  Status HelloReq(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override {

    std::string prefix ("hello ");
    reply->set_message(prefix+ request->name());
    return Status::OK;
  }
};


int main() {
  std::string server_addr("localhost:50001");
  HelloworldService service;

  ServerBuilder builder;
  builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout<< "Server listening on " << server_addr<<std::endl;
  server->Wait();

  return 0;
}
