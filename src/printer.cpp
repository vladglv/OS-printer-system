/***
 *
 * Vlad Glv
 *
 * ECSE-427 (Operating Systems)
 *
 * Assignment 2
 *
 ***/

#include "common.hpp"

printer_system* psys = nullptr;

void handler_server(int signo) {
    UNUSED(signo);

    ERR_NULL2(psys, "nullptr", exit(EXIT_FAILURE));

    // - Prevent a deadlock on crashing
    int temp;
    ERR_GEN(sem_getvalue(&psys->protect, &temp), 0);
    if(temp != 1)
        ERR_GEN(sem_post(&psys->protect), 0);

    decltype(psys->server_num) rs;

    ERR_GEN(sem_wait(&psys->protect), 0);
    psys->server_num--;
    ERR_GEN(sem_post(&psys->protect), 0);

    rs = psys->server_num;

    // - Release resources if it is the last server closing
    if(rs < 1) {
        release_shared_memory_printer(&psys);
        ERR_GEN(detach_shared_memory(&psys), true);
        ERR_GEN(shm_unlink(MY_PRSYS.c_str()), 0);
    } else
        ERR_GEN(detach_shared_memory(&psys), true);

    exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);

    ERR_GENI(signal(SIGINT, handler_server), SIG_ERR);
    ERR_GENI(signal(SIGABRT, handler_server), SIG_ERR);
    ERR_GENI(signal(SIGTERM, handler_server), SIG_ERR);
    ERR_GENI(signal(SIGHUP, handler_server), SIG_ERR);

    int fd = -1;
    printer_system* shared_mem = nullptr;

    auto f = !shm_exists(MY_PRSYS);
    if(f) {
        setup_shared_memory(fd, MY_PRSYS, SERVER_FLAGS, DEFAULT_PERMS, true);
        attach_shared_memory(fd, &shared_mem);
        init_shared_memory_printer(&shared_mem);
    } else {
        ERR_GEN_EXIT(
            setup_shared_memory(fd, MY_PRSYS, CLIENT_FLAGS, DEFAULT_PERMS),
            true);
        ERR_GEN_EXIT(attach_shared_memory(fd, &shared_mem), true);
    }

    psys = shared_mem;
    ERR_NULL2(psys, "nullptr", exit(EXIT_FAILURE));

    auto server_id = shared_mem->server_num;

    ERR_GEN(sem_wait(&shared_mem->protect), 0);
    shared_mem->server_num++;
    ERR_GEN(sem_post(&shared_mem->protect), 0);

    while(1) {
        job j;
        if(dequeue(&shared_mem, j)) {
            std::printf("Printer %d starts printing %d pages from client %d\n",
                        server_id,
                        j.page_num,
                        j.job_id);
            sleep(j.page_num);
            std::printf(
                "Printer %d finishes printing %d pages from client %d\n",
                server_id,
                j.page_num,
                j.job_id);
        }
    }

    return EXIT_SUCCESS;
}
