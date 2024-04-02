#include <iostream>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include "chat.pb.h"

using namespace std;

// Estructura para almacenar la informacion de un cliente
struct Client {
    int socket;
    string username;
    char ip[INET_ADDRSTRLEN];
    string status;
};

// Lista de clientes conectados
list<Client> connectedClients;


// Manejador de peticiones
void *handleRequests(void *args) {
    Client *client = (Client *)args;
    int socket = client->socket;
    char buffer[2048];

    // estructura del servidor
    string serverMessage;
    chat::ClientPetition *request = new chat::ClientPetition();
    chat::ServerResponse *response = new chat::ServerResponse();

    while (true) {
        response->Clear(); // Limpiar la respuesta enviada
        int bytes_received = recv(socket, buffer, 2048, 0);

        if (bytes_received <= 0) {
            // Eliminar el cliente de clientes conectados
            connectedClients.remove_if([client](const Client &c) { return c.username == client->username; });
            cout << "Usuario: " << client->username << " removido de la sesion" << endl;
            break;
        }

        if (!request->ParseFromString(buffer)) {
            cout << "Error en el parseo" << endl;
            break;
        } else {
            cout << "Opcion Recibida:" << request->option() << endl;
        }

        // Evaluar la opcion de la peticion
        switch (request->option()) {

            // Registro de nuevo usuario
            case 1: { 
                string username = request->registration().username();
                auto it = find_if(connectedClients.begin(), connectedClients.end(), [&username](const Client& c) {
                    return c.username == username;
                });

                if (it != connectedClients.end()) {
                    response->Clear();
                    response->set_option(1);
                    response->set_servermessage("FALLO: Usuario ya existe");
                    response->set_code(500);
                    response->SerializeToString(&serverMessage);
                    strcpy(buffer, serverMessage.c_str());
                    send(socket, buffer, serverMessage.size() + 1, 0);
                    cout << "ERROR: No se pudo agregar usuario al socket: " << socket << " porque ya existe" << endl;
                } else {
                    Client client;
                    client.socket = socket;
                    client.username = username;
                    client.status = "activo";
                    strcpy(client.ip, newClient->ip);
                    connectedClients.push_back(client);

                    response->Clear();
                    response->set_option(1);
                    response->set_servermessage("EXITO: usuario registrado");
                    response->set_code(200);
                    response->SerializeToString(&serverMessage);
                    strcpy(buffer, serverMessage.c_str());
                    send(socket, buffer, serverMessage.size() + 1, 0);

                    cout << "Usuario " << client.username << " agregado al sistema con socket: " << socket << endl;
                }
                break;
            }

            // Solicitud de usuarios conectados
            case 2: {
                // Verificar que no se especifique un unico usuario en esta solicitud
                if (request->users().user() != "everyone"){
                    response->Clear();
                    response->set_option(2);
                    response->set_code(500);
                    response->set_servermessage("FALLO: Usuario especifico en solicitud general");
                    response->SerializeToString(&serverMessage);
                    strcpy(buffer, serverMessage.c_str());
                    send(socket, buffer, serverMessage.size() + 1, 0);
                    cout << "ERROR: No se puede solicitar un usuario especifico al solicitar todos los usuarios" << endl;
                    break;
                }
                else{
                    chat::ConnectedUsersResponse *users = new chat::ConnectedUsersResponse();
                    for (const auto& client : connectedClients) {
                        chat::UserInfo *user_info = users->add_connectedusers();
                        user_info->set_username(client.username);
                        user_info->set_status(client.status);
                        user_info->set_ip(client.ip);
                    }
                    response->Clear();
                    response->set_servermessage("EXITO: Listado de usuarios conectados");
                    response->set_allocated_connectedusers(users);
                    response->set_option(2);
                    response->set_code(200);
                    response->SerializeToString(&serverMessage);
                    strcpy(buffer, serverMessage.c_str());
                    send(socket, buffer, serverMessage.size() + 1, 0);

                    cout << "Usuario: " << newClient->username << " solicito todos los usuarios" << endl;
                }
                break;
            }

            // Cambiar estado del usuario
            case 3: {
                string username = request->change().username();
                auto it = find_if(connectedClients.begin(), connectedClients.end(), [&username](Client& c) {
                    return c.username == username;
                });

                if (it != connectedClients.end()) {
                    it->status = request->change().status();
                    response->Clear();
                    response->set_option(3);
                    response->set_servermessage("EXITO: Estado cambiado");
                    response->set_code(200);
                    chat::ChangeStatus *sStatus = new chat::ChangeStatus();
                    sStatus->set_username(it->username);
                    sStatus->set_status(it->status);
                    response->set_allocated_change(sStatus);
                    response->SerializeToString(&serverMessage);
                    strcpy(buffer, serverMessage.c_str());
                    send(socket, buffer, serverMessage.size() + 1, 0);

                    cout << "Usuario: " << it->username << " cambio su estado a " << it->status << endl;
                } else {
                    response->Clear();
                    response->set_option(3);
                    response->set_servermessage("FALLO: Usuario no existe");
                    response->set_code(500);
                    response->SerializeToString(&serverMessage);
                    strcpy(buffer, serverMessage.c_str());
                    send(socket, buffer, serverMessage.size() + 1, 0);

                    cout << "ERROR: No se pudo cambiar estado del usuario: " << username << " porque no existe" << endl;
                }
                break;
            }

            // Enviar mensaje grupal o individual
            case 4: {
                string recipient = request->messagecommunication().recipient();
                string sender = request->messagecommunication().sender();
                string message_content = request->messagecommunication().message();

                if (recipient == "everyone"){
                    // Enviar el mensaje a todos los usuarios conectados, excepto el remitente
                    for (auto& client : connectedClients) {
                        if (client.username != sender) {
                            chat::MessageCommunication *message = new chat::MessageCommunication();
                            message->set_sender(sender);
                            message->set_message(message_content);
                            response->Clear();
                            response->set_option(4);
                            response->set_code(200);
                            response->set_servermessage("EXITO: Mensaje enviado a general");
                            response->set_allocated_messagecommunication(message);
                            response->SerializeToString(&serverMessage);
                            strcpy(buffer, serverMessage.c_str());
                            send(client.socket, buffer, serverMessage.size() + 1, 0);
                        }
                    }
                    auto senderIt = find_if(connectedClients.begin(), connectedClients.end(), [&sender](const Client& c) {
                        return c.username == sender;
                    });
                    // enviar confirmacion al remitente
                    if (senderIt != connectedClients.end()) {
                        chat::MessageCommunication *confrim_msg = new chat::MessageCommunication();
                        confrim_msg->set_sender("Servidor");
                        confrim_msg->set_message("Su mensaje ha sido enviado a todos los usuarios");
                        response->Clear();
                        response->set_option(4);
                        response->set_code(200);
                        response->set_servermessage("EXITO: Mensaje enviado a general");
                        response->set_allocated_messagecommunication(confrim_msg);
                        response->SerializeToString(&serverMessage);
                        strcpy(buffer, serverMessage.c_str());
                        send(senderIt->socket, buffer, serverMessage.size() + 1, 0);
                        cout << "Usuario: " << sender << " ha enviado mensaje a todos los usuarios" << endl;
                    }
                }
                // Enviar mensaje a un usuario especifico
                else {
                    auto recipientIt = find_if(connectedClients.begin(), connectedClients.end(), [&recipient](const Client& c) {
                        return c.username == recipient;
                    });

                    if (recipientIt != connectedClients.end()) {
                        // Usuario encontrado, enviar mensaje
                        chat::MessageCommunication *message = new chat::MessageCommunication();
                        message->set_sender(sender);
                        message->set_message(message_content);
                        response->Clear();
                        response->set_option(4);
                        response->set_code(200);
                        response->set_servermessage("EXITO: Mensaje directo enviado");
                        response->set_allocated_messagecommunication(message);
                        response->SerializeToString(&serverMessage);
                        strcpy(buffer, serverMessage.c_str());
                        send(recipientIt->socket, buffer, serverMessage.size() + 1, 0);

                        auto senderIt = find_if(connectedClients.begin(), connectedClients.end(), [&sender](const Client& c) {
                            return c.username == sender;
                        });

                        // enviar confirmacion al remitente
                        if (senderIt != connectedClients.end()) {
                            chat::MessageCommunication *confrim_msg = new chat::MessageCommunication();
                            confrim_msg->set_sender("Servidor");
                            confrim_msg->set_message("Su mensaje ha sido enviado a " + recipient);
                            response->Clear();
                            response->set_option(4);
                            response->set_code(200);
                            response->set_servermessage("EXITO: Mensaje directo enviado");
                            response->set_allocated_messagecommunication(confrim_msg);
                            response->SerializeToString(&serverMessage);
                            strcpy(buffer, serverMessage.c_str());
                            send(senderIt->socket, buffer, serverMessage.size() + 1, 0);

                            cout << "Usuario: " << sender << " ha enviado mensaje directo a " << recipient << endl;
                        }
                    }
                } 
                // usuario no encontrado
                else {
                    response->Clear();
                    response->set_option(4);
                    response->set_servermessage("FALLO: Usuario no encontrado");
                    response->set_code(500);
                    response->SerializeToString(&serverMessage);
                    strcpy(buffer, serverMessage.c_str());
                    send(socket, buffer, serverMessage.size() + 1, 0);
                    cout << "ERROR: No se pudo enviar mensaje directo a usuario: " << recipient << " porque no existe" << endl;
                }
            }
            // Obtener informacion de un usuario
            case 5: {
                string username = request->users().user();
                auto it = find_if(connectedClients.begin(), connectedClients.end(), [&username](const Client& c) {
                    return c.username == username;
                });

                if (it != connectedClients.end()) {
                    chat::UserInfo *userI = new chat::UserInfo();
                    userI->set_username(it->username);
                    userI->set_status(it->status);
                    userI->set_ip(it->ip);
                    response->set_option(5);
                    response->set_allocated_userinforesponse(userI);
                    response->set_servermessage("EXITO: Informacion de usuario obtenida");
                    response->set_code(200);
                    response->SerializeToString(&serverMessage);
                    strcpy(buffer, serverMessage.c_str());
                    send(socket, buffer, serverMessage.size() + 1, 0);
                } else {
                    response->Clear();
                    response->set_option(5);
                    response->set_servermessage("FALLO: Usuario no encontrado");
                    response->set_code(500);
                    response->SerializeToString(&serverMessage);
                    strcpy(buffer, serverMessage.c_str());
                    send(socket, buffer, serverMessage.size() + 1, 0);
                    cout << "ERROR: No se pudo obtener informacion de usuario: " << username << " porque no existe" << endl;
                }
                break;
            }
            default: {
                break;
            }
        }
    }
    return nullptr;
}