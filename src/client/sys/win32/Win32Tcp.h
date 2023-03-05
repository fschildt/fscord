class Win32Tcp
{
public:
    Win32Tcp();
    ~Win32Tcp();

    bool Connect(const char *addr, u16 port);
    void Disconnect();
    int Recv(void *buff, int size);
    bool Send(void *buff, int size);

private:
    SOCKET m_Socket;
};
