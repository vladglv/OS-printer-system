/***
 *
 * Vlad Glv
 *
 * ECSE-427 (Operating Systems)
 *
 * Assignment 2
 *
 ***/

#ifndef _INCLUDE_COMMON_HPP_
#define _INCLUDE_COMMON_HPP_

// - Standard C++
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

// - System specific
#include <unistd.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h> //!< For mode constants
#include <fcntl.h>    //!< For O_* constants
#include <signal.h>
#include <semaphore.h>

const std::string MY_PRSYS = "/PrinterSystem"; //!< Shared memory object
const std::uint16_t MAX_JOBS = 10;             //!< Size of the job pool
const int CLIENT_FLAGS = O_RDWR;               //!< Client flags
const int SERVER_FLAGS = O_CREAT | O_RDWR;     //!< Server flags
const mode_t DEFAULT_PERMS = S_IRWXU;          //!< Client/Server permissions

// - Macro to inhibit unused variable warnings
#define UNUSED(expr)  \
    do {              \
        (void)(expr); \
    } while(0)

// - Macro for generic error handling with (expr == val)
#define ERR_GEN(expr, val)                                  \
    do {                                                    \
        auto b = (expr == val);                             \
        if(!b) {                                            \
            std::printf("Error : function '%s' in %s:%d\n", \
                        __func__,                           \
                        __FILE__,                           \
                        __LINE__);                          \
            assert(b);                                      \
        }                                                   \
    } while(0)

// - Macro for generic error handling with (expr != val)
#define ERR_GENI(expr, val)                                 \
    do {                                                    \
        auto b = (expr != val);                             \
        if(!b) {                                            \
            std::printf("Error : function '%s' in %s:%d\n", \
                        __func__,                           \
                        __FILE__,                           \
                        __LINE__);                          \
            assert(b);                                      \
        }                                                   \
    } while(0)

// - Macro for generic error handling with (expr == val) and exit
#define ERR_GEN_EXIT(expr, val)                             \
    do {                                                    \
        auto b = (expr == val);                             \
        if(!b) {                                            \
            std::printf("Error : function '%s' in %s:%d\n", \
                        __func__,                           \
                        __FILE__,                           \
                        __LINE__);                          \
            exit(EXIT_FAILURE);                             \
        }                                                   \
    } while(0)

// - Macro for nullptr handling with assert, an error message and an action
#define ERR_NULL(var, msg)                    \
    do {                                      \
        if(!var) {                            \
            std::printf("Error : %s\n", msg); \
            assert(var != nullptr);           \
        }                                     \
    } while(0)

// - Macro for nullptr handling with assert, an error message and an action
#define ERR_NULL2(var, msg, action)           \
    do {                                      \
        if(!var) {                            \
            std::printf("Error : %s\n", msg); \
            assert(var != nullptr);           \
            action;                           \
        }                                     \
    } while(0)

// - Macro for nullptr handling with assert, an error message and a return
#define ERR_NULL3(var, msg, rt) ERR_NULL2(var, msg, return rt);

/**
 * @brief Structure defining a job
 */
typedef struct _job {
    std::uint16_t page_num; //!< Number of pages in a document to print
    std::uint16_t job_id;   //!< ID of the job
} job;

/**
 * @brief Structure defining the state of the printer system
 */
typedef struct _printer_system {
    sem_t protect;            //!< Semaphore protecting the buffer
    sem_t full;               //!< Semaphore protecting the client
    sem_t empty;              //!< Semaphore protecting the server
    std::uint16_t client_num; //!< Counter for clients
    std::uint16_t server_num; //!< Number of active printers
    std::uint16_t jobs_idx;   //!< Index of the newest jobs
    job job_list[MAX_JOBS];   //!< Structure holding the list of jobs
} printer_system;

/**
 * @brief Set-up a memory object with an identifier of 'fd' and name of 'name'
 * @param[out]  fd      File descriptor associated with the shared memory object
 * @param[in]   name    Name of the shared memory object
 * @param[in]   flags   Flags used to open the shared memory object
 * @param[in]   perms   Permissions used to open the shared memory object
 * @param[in]   clear   Clear the shared memory on true or preserve on false
 * @return True if the creation of the shared memory succeded or false otherwise
 */
bool setup_shared_memory(int& fd,
                         std::string name,
                         int flags,
                         mode_t perms,
                         bool clear = false) {
    fd = shm_open(name.c_str(), flags, perms);
    if(fd == -1)
        return false;

    if(clear)
        ERR_GEN(ftruncate(fd, sizeof(printer_system)), 0);

    return true;
}

/**
 * @brief Maps shared memory to the virtual space of this process
 * @param[in]   fd  File descriptor associated with the shared memory object
 * @param[out]  shm A non-null pointer which will be associated with the mapped
 * region
 * @return True on success and false otherwise
 */
bool attach_shared_memory(int fd, printer_system** shm) {
    ERR_NULL3(shm, "nullptr", false);

    *shm = (printer_system*)mmap(nullptr,
                                 sizeof(printer_system),
                                 PROT_READ | PROT_WRITE,
                                 MAP_SHARED,
                                 fd,
                                 0);
    if(*shm == MAP_FAILED)
        return false;

    return true;
}

/**
 * @brief Removes the mapping to the shared memory
 * @param[in] shm A non-null pointer which will be associated with the mapped
 * region
 * @return True on success and false otherwise
 */
bool detach_shared_memory(printer_system** shm) {
    ERR_NULL3(shm, "nullptr", false);

    auto r = munmap(*shm, sizeof(printer_system));
    if(r == -1)
        return false;

    return true;
}

/**
 * @brief Checks whether a shared memory object exists
 * @param[in] name Name of the shared memory object
 * @return True on success and false otherwise
 */
bool shm_exists(std::string name) {
    int fd;
    auto r = setup_shared_memory(fd, name, CLIENT_FLAGS, DEFAULT_PERMS);
    if(r) {
        assert(close(fd) == 0);
        return true;
    }

    return false;
}

/**
 * @brief Initiazes the printer data and semaphores
 * @param[in] shm A non-null pointer which will be associated with the mapped
 * region
 */
void init_shared_memory_printer(printer_system** shm) {
    ERR_NULL(shm, "nullptr");

    auto sm = *shm;

    sm->server_num = 0;
    sm->client_num = 0;
    sm->jobs_idx = 0;

    std::memset(&sm->job_list[0], 0, sizeof(sm->job_list));

    ERR_GEN(sem_init(&sm->protect, 1, 1), 0);
    ERR_GEN(sem_init(&sm->full, 1, 0), 0);
    ERR_GEN(sem_init(&sm->empty, 1, MAX_JOBS), 0);
}

/**
 * @brief Releases the printer data and semaphores
 * @param[in] shm A non-null pointer which will be associated with the mapped
 * region
 */
void release_shared_memory_printer(printer_system** shm) {
    ERR_NULL(shm, "nullptr");

    auto sm = *shm;

    ERR_GEN(sem_destroy(&sm->protect), 0);
    ERR_GEN(sem_destroy(&sm->full), 0);
    ERR_GEN(sem_destroy(&sm->empty), 0);
}

/**
 * @brief Enqueues a job 'j' to a shared buffer
 * @param[in,out] shm A non-null pointer which will be associated with the
 * mapped region
 * @param[in]     j   Job to enqueue
 * @return True on success and false otherwise
 */
bool enqueue(printer_system** shm, job& j) {
    ERR_NULL3(shm, "nullptr", false);

    auto sm = *shm;

    ERR_GEN(sem_wait(&sm->empty), 0);
    ERR_GEN(sem_wait(&sm->protect), 0);
    std::memcpy(&sm->job_list[sm->jobs_idx++], &j, sizeof(job));
    ERR_GEN(sem_post(&sm->protect), 0);
    ERR_GEN(sem_post(&sm->full), 0);

    return true;
}

/**
 * @brief Dequeues a job 'j' from a shared buffer
 * @param[in,out] shm A non-null pointer which will be associated with the
 * mapped region
 * @param[in,out] j   Job to enqueue
 * @return True on success and false otherwise
 */
bool dequeue(printer_system** shm, job& j) {
    ERR_NULL3(shm, "nullptr", false);

    auto sm = *shm;

    ERR_GEN(sem_wait(&sm->full), 0);
    ERR_GEN(sem_wait(&sm->protect), 0);
    std::memcpy(&j, &sm->job_list[--sm->jobs_idx], sizeof(job));
    ERR_GEN(sem_post(&sm->protect), 0);
    ERR_GEN(sem_post(&sm->empty), 0);

    return true;
}

#endif //_INCLUDE_COMMON_HPP_
