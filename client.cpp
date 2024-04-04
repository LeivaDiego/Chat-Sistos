#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include "chat.pb.h"

using namespace std;
std::unordered_map<string,thread> privates;

void listenPrivateMessages(int sockfd,string username){
    while (true)
    {
        char buffer[8192];
        int bytes_recibidos = recv(sockfd, buffer, 8192, 0);
        if (bytes_recibidos == -1) {
            perror("recv fallido");
            break;
        } else if (bytes_recibidos == 0) {
            break;
        } else {
            chat::ServerResponse *respuesta=new chat::ServerResponse();
            if (respuesta->option()==4){ //este thread solo manejara mensajes privados del username
            if (respuesta->code() != 200) {
                std::cout << respuesta->servermessage()<< std::endl;
                break;
            }
            if (respuesta->messagecommunication().sender()==username){
                // Mostrar mensaje recibido
                cout <<"** Mensaje Directo **" << endl;
                cout << "Mensaje del servidor: " << respuesta->servermessage() << endl;
                cout<<respuesta->messagecommunication().sender() << ":\n"<< respuesta->messagecommunication().message() << endl;
            }}
        }
    }
    
}
void listenResponses(int sockfd){
    while (true) {
        char buffer[8192];
        int bytes_recibidos = recv(sockfd, buffer, 8192, 0);
        if (bytes_recibidos == -1) {
            perror("recv fallido");
            break;
        } else if (bytes_recibidos == 0) {
            cout << "Error en conexión del servidor" << endl;
            break;
        } else {

            chat::ServerResponse *respuesta=new chat::ServerResponse();
            if (!respuesta->ParseFromString(buffer)) {
                cerr << "Error parseando respuesta.\n" << endl;
                break;
            }

            if (respuesta->code() != 200) {
                std::cout << respuesta->servermessage()<< std::endl;
                return;
            }
            switch (respuesta->option())
            {
            case 2: 
                cout<<"** Usuarios Conectados **"<<endl;
                for (int i = 0; i < respuesta->connectedusers().connectedusers_size(); ++i) {
                    auto user = respuesta->connectedusers().connectedusers(i);
                    cout<< "Usuario: " << user.username()<<endl;
                }

                break;
            case 3:
                cout<<"** Cambio de Estado **"<<endl;
                cout<<respuesta->servermessage()<<endl;
            break;
            case 4: //este thread solo manejara mensajes publicos, no privados , solo imprime si no existe hilo para dicho chat con el user
            if (!respuesta->messagecommunication().has_recipient()||respuesta->messagecommunication().recipient()=="everyone"){
                // Mostrar mensaje recibido
                cout <<"** Transmision Global **" << endl;
                cout << "Mensaje del servidor: " << respuesta->servermessage() << endl;
                cout<<respuesta->messagecommunication().sender() << ":\n"<< respuesta->messagecommunication().message() << endl;
            }
            else if(privates.find(respuesta->messagecommunication().sender()) == privates.end()){//no existe el thread
                cout <<"** Transmision Privada **" << endl;
                cout << "Mensaje del servidor: " << respuesta->servermessage() << endl;
                cout<<respuesta->messagecommunication().sender() << ":\n"<< respuesta->messagecommunication().message() << endl;
                //crear el thread para la transmision privada
                privates[respuesta->messagecommunication().sender()] = thread(listenPrivateMessages,sockfd,respuesta->messagecommunication().sender());
            }
            break;
            case 5:
                    cout<<"** Informacion del Usuario **"<<endl;
                    cout<<"Usuario: ";
                    cout<<respuesta->userinforesponse().username();
                    cout<<"\nEstatus: ";
                    cout<<respuesta->userinforesponse().status();
                    cout<<"\nIP: ";
                    cout<<respuesta->userinforesponse().ip()<<endl;
            break;
            default:
                cout<<"Respuesta Desconocida"<<endl;
                break;
            }
        }
    }
}

void enviarMensaje(int sockfd, const string& mensaje, const string& remitente = "",const string& destinatario = "everyone") {
    char buffer[8192];
    chat::ClientPetition *request = new chat::ClientPetition();
    chat::MessageCommunication *mensaje_comunicacion = new chat::MessageCommunication();
    mensaje_comunicacion->set_message(mensaje);
    mensaje_comunicacion->set_recipient(destinatario);
    mensaje_comunicacion->set_sender(remitente);
    request->set_option(4);
    request->set_allocated_messagecommunication(mensaje_comunicacion);
    std::string mensaje_serializado;
    if(!request->SerializeToString(&mensaje_serializado)){
        cerr << "Error de serializacion (mensaje)" << endl;
        return;
    };
	strcpy(buffer, mensaje_serializado.c_str());
	send(sockfd, buffer, mensaje_serializado.size() + 1, 0);
}

void chateoPrivado(int sockfd, const string& destinatario,const string& remitente,const string& mensaje) {
    char buffer[8192];
    chat::ClientPetition *request = new chat::ClientPetition();
    chat::MessageCommunication *mensaje_comunicacion = new chat::MessageCommunication();
    mensaje_comunicacion->set_message(mensaje);
    mensaje_comunicacion->set_recipient(destinatario);
    mensaje_comunicacion->set_sender(remitente);
    request->set_option(4);
    request->set_allocated_messagecommunication(mensaje_comunicacion);
    std::string mensaje_serializado;
    if(!request->SerializeToString(&mensaje_serializado)){
        cerr << "Error de serializacion (mensaje)" << endl;
        return;
    };
	strcpy(buffer, mensaje_serializado.c_str());
	if(!send(sockfd, buffer, mensaje_serializado.size() + 1, 0)){perror("send failed");};
}

void cambiarEstado(int sockfd, const string& username, const string& estado) {
    char buffer[8192];
    chat::ClientPetition *request = new chat::ClientPetition();
    chat::ChangeStatus *cambio_estado = new chat::ChangeStatus;
    cambio_estado->set_username(username);
    cambio_estado->set_status(estado);
    request->set_option(3);
    request->set_allocated_change(cambio_estado);

    string cambio_estado_serializado;
    if (!request->SerializeToString(&cambio_estado_serializado)) {
        cerr << "Error de serializacion (Estado)" << endl;
        return;
    }
    strcpy(buffer, cambio_estado_serializado.c_str());
    if (send(sockfd, buffer, cambio_estado_serializado.size()+1, 0) == -1) {
        perror("Error envio");
    }
}

void listarUsuarios(int sockfd) {
    char buffer[8192];
    chat::ClientPetition *peticion = new chat::ClientPetition();
    peticion->set_option(2); // Opción para listar usuarios conectados
    string peticion_serializada;
    if (!peticion->SerializeToString(&peticion_serializada)) {
        cerr << "Error de serializacion (Lista Usuarios)" << endl;
        return;
    }
    strcpy(buffer, peticion_serializada.c_str());
	if(!send(sockfd, buffer, peticion_serializada.size() + 1, 0)){perror("Error envio");};
}

void obtenerInfoUsuario(int sockfd, const string& nombre_usuario) {
   char buffer[8192];
    chat::ClientPetition *peticion = new chat::ClientPetition();
    string peticion_serializada;
    chat::UserRequest *user_req = new chat::UserRequest();
    user_req->set_user(nombre_usuario);
    peticion->set_option(5); // Opción para listar usuarios conectados
    peticion->set_allocated_users(user_req);
    if (!peticion->SerializeToString(&peticion_serializada)) {
        cerr << "Error de serializacion (Info de Usuario)" << endl;
        return;
    }
    strcpy(buffer, peticion_serializada.c_str());
    if(!send(sockfd, buffer, peticion_serializada.size() + 1, 0)){perror("Error envio");};    
}

int main(int argc, char const *argv[]) {
    if (argc != 4) {
        cerr << "Uso: " << argv[0] << " <nombredeusuario> <IP> <puerto>" << endl;
        return 1;
    }

    string nombre_usuario = argv[1];
    string ip_servidor = argv[2];
    int puerto_servidor = stoi(argv[3]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Error en socket");
        return 1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(puerto_servidor);
    inet_pton(AF_INET, ip_servidor.c_str(), &server_addr.sin_addr);

    if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect fallido");
        return 1;
    }

    std::string message_serialized;
    chat::ClientPetition *request = new chat::ClientPetition();
    chat::ServerResponse *response = new chat::ServerResponse();
    chat::UserRegistration *new_user = new chat::UserRegistration();

    new_user->set_ip("10.0.2.15");
    new_user->set_username(nombre_usuario);
    request->set_option(1);
    request->set_allocated_registration(new_user);
    char buffer[8192];

    request->SerializeToString(&message_serialized);
	strcpy(buffer, message_serialized.c_str());
	send(sockfd, buffer, message_serialized.size() + 1, 0);
	recv(sockfd, buffer, 8192, 0);
	response->ParseFromString(buffer);

	if (response->code() != 200) {
		std::cout << response->servermessage()<< std::endl;
		return 0;
	}
    std::cout << "Servidor: "<< response->servermessage()<< std::endl;
    cout << "¡Conectado al servidor!" << endl;
    
    thread recibir_thread(listenResponses, sockfd);
    recibir_thread.detach();

    string opcion;
    while (true) {
        cout << "============================================================";
        cout << "\n================== Bienvenido al chat UVG ==================\n";
        cout << "============================================================\n";
        cout << "| 1. Chatear con todos los usuarios (broadcasting)         |\n";
        cout << "| 2. Enviar y recibir mensajes directos, privados          |\n";
        cout << "| 3. Cambiar de status                                     |\n";
        cout << "| 4. Listar los usuarios conectados al sistema de chat     |\n";
        cout << "| 5. Desplegar informacion de un usuario en particular     |\n";
        cout << "| 6. Ayuda                                                 |\n";
        cout << "| 7. Salir                                                 |\n";
        cout << "============================================================\n";
        cout << "Por favor, elige una opcion: ";
        cin >> opcion;

        

        if (opcion == "1") {
            string message;
            cout << "** Pon el mensaje que deseas enviar **"<<endl;
            cin>>message;
            enviarMensaje(sockfd,message,nombre_usuario);
           
        } else if (opcion == "2") {
            
            string destin;
            cout<<"** A quien deseas enviarle el mensaje (ingresa su username) **"<<endl;
            cin>>destin;
            string message;
            cout << "** Que mensaje deseas enviar **"<<endl;
            cin>>message;
            chateoPrivado(sockfd,destin,nombre_usuario,message);
            privates[destin] = thread(listenPrivateMessages,sockfd,destin);
        } else if (opcion == "3") {
            cout << "** Ingresa el estado en el que te encuentras **";
            cout << "** Activo **";
            cout << "** Inactivo **";
            cout << "** Ocupado **";
            string estado;
            cin >> estado;
            cambiarEstado(sockfd, nombre_usuario,estado);
        } else if (opcion == "4") {
            listarUsuarios(sockfd);
        } else if (opcion == "5") {
            cout << "** De que usuario deseas saber su informacion (ingresa su username) **";
            string nombre_usuario;
            cin >> nombre_usuario;
            obtenerInfoUsuario(sockfd, nombre_usuario);
        } else if (opcion == "6") {
                cout << "================= Ayuda del Sistema de Chat =================" << endl;
                cout << "A continuación, se presentan las opciones disponibles y cómo usarlas:" << endl;
                cout << "1. Chatear con todos los usuarios: Envía un mensaje a todos los usuarios conectados." << endl;
                cout << "   Uso: Seleccione la opción 1 y escriba su mensaje." << endl;
                cout << "2. Enviar mensaje directo: Envía un mensaje privado a un usuario específico." << endl;
                cout << "   Uso: Seleccione la opción 2, ingrese el nombre del usuario y luego su mensaje." << endl;
                cout << "3. Cambiar estado: Cambia tu estado en el chat (por ejemplo, en línea, ocupado)." << endl;
                cout << "   Uso: Seleccione la opción 3 y siga las instrucciones para cambiar su estado." << endl;
                cout << "4. Listar usuarios: Muestra una lista de todos los usuarios conectados." << endl;
                cout << "   Uso: Seleccione la opción 4 para ver la lista de usuarios conectados." << endl;
                cout << "5. Información de usuario: Obtiene información detallada sobre un usuario específico." << endl;
                cout << "   Uso: Seleccione la opción 5 y escriba el nombre del usuario que desea consultar." << endl;
                cout << "6. Ayuda: Muestra esta guía de ayuda." << endl;
                cout << "   Uso: Seleccione la opción 6 para ver nuevamente esta guía de ayuda." << endl;
                cout << "7. Salir: Desconecta del chat y cierra la aplicación." << endl;
                cout << "   Uso: Seleccione la opción 7 para salir del sistema de chat." << endl;
                cout << "============================================================" << endl;
        } else if (opcion == "7") {
            break;
        } else {
            cout << "Opcion Invalida" << endl;
        }
    }

    close(sockfd);

    return 0;
}
