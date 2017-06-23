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

void handler_client(int signo) {
    UNUSED(signo);

    ERR_NULL2(psys, "nullptr", exit(EXIT_FAILURE));

    // - Prevent a deadlock on crashing
    int temp;
    ERR_GEN(sem_getvalue(&psys->protect, &temp), 0);
    if(temp != 1)
        ERR_GEN(sem_post(&psys->protect), 0);

    exit(EXIT_SUCCESS);
}

int main(int argc, char** argv) {
    UNUSED(argc);
    UNUSED(argv);

    ERR_GENI(signal(SIGINT, handler_client), SIG_ERR);
    ERR_GENI(signal(SIGABRT, handler_client), SIG_ERR);
    ERR_GENI(signal(SIGTERM, handler_client), SIG_ERR);
    ERR_GENI(signal(SIGHUP, handler_client), SIG_ERR);

    int fd = -1;
    printer_system* shared_mem = nullptr;
    int pages = 0;

    if(argc == 2) {
        try {
            pages = std::stoi(argv[1]);
            if(pages < 1)
                throw std::runtime_error("Negative number of pages");
        } catch(...) {
            std::printf("Invalid number of pages\n");
            exit(EXIT_FAILURE);
        }
    } else {
        std::printf("Please provide a number of pages as the only argument\n");
        std::printf("Syntax: ./client num_pages[int]\n");
        exit(EXIT_FAILURE);
    }

    if(!shm_exists(MY_PRSYS)) {
        std::printf("No server(s) are currently running\n");
        exit(EXIT_FAILURE);
    }

    ERR_GEN_EXIT(setup_shared_memory(fd, MY_PRSYS, CLIENT_FLAGS, DEFAULT_PERMS),
                 true);
    ERR_GEN_EXIT(attach_shared_memory(fd, &shared_mem), true);

    psys = shared_mem;
    ERR_NULL2(psys, "nullptr", exit(EXIT_FAILURE));

    ERR_GEN(sem_wait(&shared_mem->protect), 0);
    shared_mem->client_num++;
    ERR_GEN(sem_post(&shared_mem->protect), 0);

    job j;
    j.job_id = getpid();
    j.page_num = pages;

    if(enqueue(&shared_mem, j)) {
        std::printf("Client %d has %d pages to print, puts request in Buffer\n",
                    j.job_id,
                    j.page_num);
    }

    ERR_GEN(sem_wait(&shared_mem->protect), 0);
    shared_mem->client_num--;
    ERR_GEN(sem_post(&shared_mem->protect), 0);

    ERR_GEN(detach_shared_memory(&psys), true);

    return EXIT_SUCCESS;
}
