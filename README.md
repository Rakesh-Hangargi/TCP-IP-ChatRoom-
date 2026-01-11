# TCP Chat Application (C, Sockets, Threads)

[![Language](https://img.shields.io/badge/language-C-blue.svg)]()  
[![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)]()  
[![Build](https://img.shields.io/badge/build-GCC%20%7C%20Make-brightgreen.svg)]()  
[![Networking](https://img.shields.io/badge/network-TCP%2FIP-orange.svg)]()  
[![Threads](https://img.shields.io/badge/concurrency-pthreads-informational.svg)]()  
[![License](https://img.shields.io/badge/license-MIT-green.svg)]()  

A multi-client chat application built in **C** using **TCP sockets**, **POSIX threads**, and a **select() multiplexed server**.  
Supports **Signup/Login**, **Group Chat**, and **Private Messaging** with real-time concurrent communication.

---

# Table of Contents

1. [Features](#features)  
2. [Architecture Overview](#architecture-overview)  
3. [Communication Protocol](#communication-protocol)  
4. [Project Structure](#project-structure)  
5. [Build Instructions](#build-instructions)  
6. [Run Instructions](#run-instructions)  
7. [Usage Guide](#usage-guide)  
8. [Sequence Diagram](#sequence-diagram)  
9. [Examples](#examples)  
10. [Future Enhancements](#future-enhancements)  
11. [License](#license)

---

# Features

### ✔ User Authentication
- Signup  
- Login  
- Username duplication detection  
- Wrong password detection  
- Username-not-found handling  

### ✔ Group Chat
- Broadcast messages to all connected users  
- Sender sees: `You : message`  
- Others see: `username : message`

### ✔ Private Chat
- Direct messaging using target client FD  
- Server routes the message correctly

### ✔ Concurrency
- Server handles multiple clients using `select()`  
- Client uses 2 threads (send + receive)

### ✔ Clean Message Protocol
```
SIGNUP|username|password
LOGIN |username|password
GROUP |username|message
USER  |username|target_fd|message
```

---

# Architecture Overview

## 1. High-Level Client–Server Architecture

```
                 ┌──────────────────────┐
                 │      Chat Server     │
                 │  (select + sockets)  │
                 └──────────┬───────────┘
                            │
        ┌───────────────────┼──────────────────────┐
        │                   │                      │
┌─────────────┐     ┌─────────────┐        ┌─────────────┐
│   Client 1   │     │   Client 2   │        │   Client N   │
│ (threads)    │     │ (threads)    │        │ (threads)    │
│ send + recv  │     │ send + recv  │        │ send + recv  │
└─────────────┘     └─────────────┘        └─────────────┘
```

---

## 2. Server Internal Architecture

```
┌──────────────────────────────────────────────────────────┐
│                         SERVER                           │
│                                                          │
│  ┌───────────────┐     ┌────────────────────────────┐    │
│  │ Listening FD   │     │    Registered Users        │    │
│  │   (port 6333)  │     │ logins[] = name, pass, fd  │    │
│  └───────┬───────┘     └────────────────────────────┘    │
│          │                           │                    │
│  ┌───────▼───────────────────────────▼────────────────┐   │
│  │                   select() Loop                    │   │
│  │                                                    │   │
│  │ Accept new clients                                 │   │
│  │ Read messages from active clients                  │   │
│  │ Parse command type                                 │   │
│  │ Route to group / target user                       │   │
│  └────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────┘
```

---

## 3. Client Architecture

```
┌─────────────────────────────────────────────┐
│                  CLIENT                     │
│                                             │
│  ┌───────────────┐   ┌─────────────────┐    │
│  │ Sending Thread │   │ Receiving Thread│    │
│  │ (scanf input)  │   │  (recv server)  │    │
│  └───────┬────────┘   └───────┬────────┘    │
│          │                    │              │
│          └──────────┬─────────┘              │
│                     │                        │
│           ┌─────────▼────────┐               │
│           │   TCP Socket     │               │
│           │  to the server   │               │
│           └──────────────────┘               │
└─────────────────────────────────────────────┘
```

---

# Communication Protocol

All messages follow a predictable structure with pipes `|` as delimiters.

### Signup
```
SIGNUP|username|password
```

### Login
```
LOGIN|username|password
```

### Group Chat
```
GROUP|username|message
```

### Private Messaging
```
USER|username|target_fd|message
```

---

# Project Structure

```
tcp-chat-app/
│
├── server/
│   ├── server.c
│   └── Makefile
│
├── client/
│   ├── client.c
│   └── Makefile
│
└── README.md
```

---

# Build Instructions

### Build Server
```bash
cd server
make
```

### Build Client
```bash
cd client
make
```

---

# Run Instructions

### Run Server
```bash
./server
```

### Run Client (in multiple terminals)
```bash
./client
```

---

# Usage Guide

### Authentication Stage

Choose:
```
1. Sign-Up
2. Login
```

Server ACK values:

| ACK | Meaning |
|-----|---------|
| 1 | Success |
| 0 | Wrong password / Signup fail |
| 2 | Username not found |
| 3 | Username already exists |

---

### Chat Mode Selection

```
1) Single Chat (Private)
2) Group Chat
```

#### Group Chat Example
```
You : hello everyone!
```
Others see:
```
rakesh : hello everyone!
```

#### Private Chat Example
```
Enter fd of user: 7
USER|rakesh|7|hello bro
```

---

# Sequence Diagram

```
 CLIENT                            SERVER
   │                                  │
   │   SIGNUP|john|1234               │
   ├─────────────────────────────────►│
   │                                  │
   │        ack = 1                   │
   ◄──────────────────────────────────┤
   │                                  │
   │   LOGIN|john|1234                │
   ├─────────────────────────────────►│
   │                                  │
   │        ack = 1                   │
   ◄──────────────────────────────────┤
   │                                  │
   │  GROUP|john|hello all            │
   ├─────────────────────────────────►│
   │                                  │
   │  (server broadcasts to clients)  │
   ◄──────────────────────────────────┤
```

---

# Examples

### Signup
```
Enter username: john
Enter password: 1234
Signup successful. Login now.
```

### Login
```
Enter username: john
Enter password: 1234
Login successful.
```

### Group Chat
```
You : hi everyone!
```

### Private Chat
```
You : hello (sent only to FD=7)
```

---

# Future Enhancements

You may extend the system with:

- Persistent user accounts (file/database)  
- Username-based private chat  
- Chat rooms / channels  
- Logout support  
- Message history  
- AES encryption  
- ncurses UI  
- File transfer support  

---

# License

This project is licensed under the **MIT License**.  
You are free to use, modify, and distribute it.

