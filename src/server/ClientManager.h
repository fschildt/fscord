#pragma once

#include <common/fscord_defs.h>
#include <common/fscord_net.h>

#include <MemoryArena.h>
#include <AesGcm.h>
#include <EpollManager.h>
class Rsa;

enum ClientStatus {
    CLIENT_DISCONNECTED,
    CLIENT_HANDSHAKE_PENDING,
    CLIENT_LOGIN_PENDING,
    CLIENT_LOGGED_IN,
};

struct Client {
    ClientStatus status;
    int fd;
    AesGcm aes_gcm;
    int name_len;
    char name[FSCN_MAX_USERNAME_LEN];
};

class ClientManager {
public:
    bool Init(Rsa *rsa, int listening_fd);
    void Run();

private:
    void HandleSocketInput(Client *client);
    void AddClient();
    void RemoveClient(Client *client);

private:
    // These must call RemoveClient if there was a failure.
    void DoLoginResponse(Client *client, int status);
    void SendUserlistTo(Client *client);
    void EncryptAndSendToEveryone();
    void EncryptAndSendToEveryoneExcept(Client *except);

private:
    // @Quality: You can make a class of this section and move a lot of code there
    bool RecvAndDecrypt(int fd, AesGcm *aes);
    bool EncryptAndSend(int fd, AesGcm *aes);

    bool RecvAesPackage(int fd);

    bool DecryptIoData(AesGcm *aes);
    bool DecryptIoData(Rsa *rsa, int decrypted_size);
    bool EncryptOutData(AesGcm *aes);

    bool SendIoData(int fd);
    bool RecvIoData(int fd, size_t size);


private:
    EpollManager m_EpollManager;
    int m_ListeningFd;
    Rsa *m_Rsa;

    MemoryArena m_ArenaIo;  // encrypted data that is sent/recvd
    MemoryArena m_ArenaIn;  // decrypted data that was recvd and decrypted
    MemoryArena m_ArenaOut; // plaintext data that will be encrypted and sent

    int16_t m_FreeClientIndicesAvail;
    int16_t m_FreeClientIndicesMax;
    int16_t *m_FreeClientIndices;
    Client *m_Clients;
};

