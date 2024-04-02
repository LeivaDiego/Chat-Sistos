#include <iostream>
#include <thread>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "chat.pb.h" 


int sockfd; // Socket descriptor

// Prototipos de funciones
void cambiarEstado();
void displayDecoratedMenu();
int getUserInput();
void processUserChoice(int choice);

void displayDecoratedMenu() {
    std::cout << "============================================================\n";
    std::cout << "\n================== Bienvenido al chat UVG ==================\n";
    std::cout << "============================================================\n";
    std::cout << "| 1. Chatear con todos los usuarios (broadcasting)         |\n";
    std::cout << "| 2. Enviar y recibir mensajes directos, privados          |\n";
    std::cout << "| 3. Cambiar de status                                     |\n";
    std::cout << "| 4. Listar los usuarios conectados al sistema de chat     |\n";
    std::cout << "| 5. Desplegar informacion de un usuario en particular     |\n";
    std::cout << "| 6. Ayuda                                                 |\n";
    std::cout << "| 7. Salir                                                 |\n";
    std::cout << "============================================================\n";
    std::cout << "Por favor, elige una opcion: ";
}

int getUserInput() {
    int choice;
    std::cin >> choice;
    return choice;
}

void enviarMensajeBroadcast() {
    std::string mensaje;
    std::cout << "Ingrese el mensaje para enviar a todos los usuarios: ";
    std::getline(std::cin >> std::ws, mensaje); // std::ws para limpiar el buffer de entrada

    chat::ClientPetition peticion;
    peticion.set_option(4); // Suponiendo que 4 es la opción para enviar un mensaje.
    
    // Configurar el mensaje de comunicación
    chat::MessageCommunication* mensajeComunicacion = peticion.mutable_messagecommunication();
    mensajeComunicacion->set_sender("tu_nombre_de_usuario"); // Deberías obtener tu nombre de usuario de alguna manera
    mensajeComunicacion->set_message(mensaje);
    // El destinatario se deja vacío o se establece a un valor especial como "todos"
    // dependiendo de la implementación del servidor.
    mensajeComunicacion->set_recipient("todos"); // Si tu servidor espera algo específico para representar "todos los usuarios"

    // Serializar y enviar la petición
    std::string serializedPeticion;
    if (!peticion.SerializeToString(&serializedPeticion)) {
        std::cerr << "Fallo al serializar la petición." << std::endl;
        return;
    }

    if (send(sockfd, serializedPeticion.data(), serializedPeticion.size(), 0) == -1) {
        perror("send fallo");
    }
}

void enviarMensajeDirecto() {
    std::string destinatario, mensaje;
    std::cout << "Ingrese el nombre del destinatario: ";
    std::getline(std::cin >> std::ws, destinatario);
    std::cout << "Ingrese el mensaje: ";
    std::getline(std::cin >> std::ws, mensaje);

    chat::ClientPetition peticion;
    peticion.set_option(4); // Suponiendo que 4 es la opción para enviar un mensaje.
    
    // Configurar el mensaje de comunicación
    chat::MessageCommunication* mensajeComunicacion = peticion.mutable_messagecommunication();
    mensajeComunicacion->set_sender("tu_nombre_de_usuario"); // Deberías obtener tu nombre de usuario de alguna manera
    mensajeComunicacion->set_recipient(destinatario); 
    mensajeComunicacion->set_message(mensaje);

    // Serializar y enviar la petición
    std::string serializedPeticion;
    if (!peticion.SerializeToString(&serializedPeticion)) {
        std::cerr << "Fallo al serializar la petición." << std::endl;
        return;
    }

    if (send(sockfd, serializedPeticion.data(), serializedPeticion.size(), 0) == -1) {
        perror("send fallo");
    }
}


void cambiarEstado() {
    std::string estado;
    std::cout << "Ingrese el nuevo estado (ACTIVO, OCUPADO, INACTIVO): ";
    std::cin >> estado;

    chat::ClientPetition petition;
    petition.set_option(3); // Asumiendo que 3 corresponde a cambiar el estado
    petition.mutable_change()->set_username("nombre_de_usuario"); // Sustituye esto con la variable que tengas para el nombre de usuario
    petition.mutable_change()->set_status(estado);

    std::string serialized_petition;
    if (petition.SerializeToString(&serialized_petition)) {
        send(sockfd, serialized_petition.data(), serialized_petition.size(), 0);
    } else {
        std::cerr << "Fallo al serializar la petición." << std::endl;
    }
}


void listarUsuarios() {
    chat::ClientPetition peticion;
    peticion.set_option(2); // Asumiendo que 2 es la opción para solicitar la lista de usuarios conectados.

    // Serializar y enviar la petición
    std::string serializedPeticion;
    if (!peticion.SerializeToString(&serializedPeticion)) {
        std::cerr << "Fallo al serializar la petición de lista de usuarios." << std::endl;
        return;
    }

    if (send(sockfd, serializedPeticion.data(), serializedPeticion.size(), 0) == -1) {
        perror("send fallo al solicitar lista de usuarios");
    }
}


void solicitarInformacionUsuario() {
    std::string nombreUsuario;
    std::cout << "Ingrese el nombre del usuario para obtener información: ";
    std::getline(std::cin >> std::ws, nombreUsuario);

    chat::ClientPetition peticion;
    peticion.set_option(5); // Asumiendo que 5 es la opción para solicitar información del usuario.
    
    // Configurar la solicitud de información de usuario
    chat::UserRequest* solicitudUsuario = peticion.mutable_users();
    solicitudUsuario->set_user(nombreUsuario);

    // Serializar y enviar la petición
    std::string serializedPeticion;
    if (!peticion.SerializeToString(&serializedPeticion)) {
        std::cerr << "Fallo al serializar la petición de información de usuario." << std::endl;
        return;
    }

    if (send(sockfd, serializedPeticion.data(), serializedPeticion.size(), 0) == -1) {
        perror("send fallo al solicitar información de usuario");
    }
}


void listenResponses(int sockfd) {
    while (true) {
        char buffer[4096]; // Asumiendo que este es el tamaño máximo de tu mensaje.
        int bytesReceived = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            // Manejar la desconexión o el error.
            break;
        }

        // Parsear la respuesta recibida del servidor.
        chat::ServerResponse respuesta;
        if (!respuesta.ParseFromArray(buffer, bytesReceived)) {
            std::cerr << "No se pudo parsear la respuesta del servidor." << std::endl;
            continue;
        }

        // Diferenciar el tipo de mensaje basado en la opción.
        if (respuesta.option() == 4 && respuesta.has_messagecommunication()) {
            if (!respuesta.messagecommunication().recipient().empty()) {
                // Manejo de mensajes directos.
                std::cout << "Mensaje directo de " << respuesta.messagecommunication().sender() << ": "
                          << respuesta.messagecommunication().message() << std::endl;
            }
        } else if (respuesta.option() == 2 && respuesta.has_connectedusers()) {
            // Manejo de la lista de usuarios conectados.
            std::cout << "\nUsuarios Conectados:\n";
            for (int i = 0; i < respuesta.connectedusers().connectedusers_size(); ++i) {
                const auto& user = respuesta.connectedusers().connectedusers(i);
                std::cout << "- " << user.username();
                if (!user.status().empty()) {
                    std::cout << " (" << user.status() << ")";
                }
                std::cout << std::endl;
            }
        } 
    }
}


void processUserChoice(int choice) {
    switch (choice) {
        case 1:
            std::cout << "\n[Chatear con todos] Opcion seleccionada.\n";
            enviarMensajeBroadcast();
            break;
        case 2:
            std::cout << "\n[Enviar y recibir mensajes directos] Opcion seleccionada.\n";
            enviarMensajeDirecto();
            break;
        case 3:
            std::cout << "\n[Cambiar de status] Opcion seleccionada.\n";
            cambiarEstado();
            break;
        case 4:
            std::cout << "\n[Listar usuarios conectados] Opcion seleccionada.\n";
            listarUsuarios();
            break;
        case 5:
            std::cout << "\n[Desplegar informacion de usuario] Opcion seleccionada.\n";
            solicitarInformacionUsuario();
            break;
        case 6:
            std::cout << "\n[Ayuda] Opción seleccionada.\n";
            // Ayuda Funcion
            break;
        case 7:
            std::cout << "\n[Salir] Saliendo del programa...\n";
            break;
        default:
            std::cout << "\nOpcion no valida. Por favor, intenta de nuevo.\n";
    }
}


int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // Código para establecer la conexión con el servidor...
    // sockfd debe ser inicializado y conectado al servidor aquí.

    // Iniciar el hilo para escuchar las respuestas del servidor
    std::thread listenerThread(listenResponses, sockfd);
    
    int choice = 0;
    while (choice != 7) {
        displayDecoratedMenu();
        choice = getUserInput();
        processUserChoice(choice);
    }

    // Antes de cerrar la aplicación, el hilo `listenerThread` debe estar terminado.
    if (listenerThread.joinable()) {
        listenerThread.join(); // Espera a que `listenerThread` termine
    }

    // Limpiar recursos de Protocol Buffers antes de salir
    google::protobuf::ShutdownProtobufLibrary();

    return 0;
}
