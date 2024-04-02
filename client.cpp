#include <iostream>
#include <thread>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "protocol.pb.h" 


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

void processUserChoice(int choice) {
    switch (choice) {
        case 1:
            std::cout << "\n[Chatear con todos] Opcion seleccionada.\n";
            enviarMensajeBroadcast();
            break;
        case 2:
            std::cout << "\n[Enviar y recibir mensajes directos] Opcion seleccionada.\n";
            enviarMensajeDirecto()
            break;
        case 3:
            std::cout << "\n[Cambiar de status] Opcion seleccionada.\n";
            cambiarEstado();
            break;
        case 4:
            std::cout << "\n[Listar usuarios conectados] Opcion seleccionada.\n";
            // Aquí iría la implementación para listar usuarios
            break;
        case 5:
            std::cout << "\n[Desplegar informacion de usuario] Opcion seleccionada.\n";
            // Aquí iría la implementación para mostrar información de usuario
            break;
        case 6:
            std::cout << "\n[Ayuda] Opción seleccionada.\n";
            // Aquí iría la implementación de ayuda
            break;
        case 7:
            std::cout << "\n[Salir] Saliendo del programa...\n";
            // Aquí iría la implementación para salir limpiamente
            break;
        default:
            std::cout << "\nOpcion no valida. Por favor, intenta de nuevo.\n";
    }
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


void listenResponses(int sockfd) {
    while (true) {
        // La lógica para recibir y parsear mensajes del servidor va aquí...
        chat::ServerResponse respuesta;
        // Asumiendo que has recibido y deserializado `respuesta` correctamente
        if (respuesta.option() == 4 && respuesta.has_messagecommunication()) {
            if (!respuesta.messagecommunication().recipient().empty()) {
                // Suponiendo que un mensaje directo tiene un destinatario especificado
                std::cout << "Mensaje directo de " << respuesta.messagecommunication().sender() << ": "
                          << respuesta.messagecommunication().message() << std::endl;
            }
        }

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
