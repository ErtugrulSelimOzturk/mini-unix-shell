#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_LINE 1024
#define MAX_ARGS 64
#define DELIM " \t\r\n\a"
#define MAX_HISTORY 10
#define MAX_JOBS 32
#define LOG_FILE "../shell.log"

// Renkler
#define COLOR_BORDO   "\x1b[38;5;88m"
#define COLOR_ORANGE  "\x1b[38;5;209m"
#define COLOR_RESET   "\x1b[0m"
#define COLOR_BOLD    "\x1b[1m"
#define COLOR_GREEN   "\x1b[1;32m"
#define COLOR_CYAN    "\x1b[38;5;45m"
#define COLOR_BLUE    "\x1b[38;5;33m"
#define COLOR_RED     "\x1b[38;5;196m"
#define COLOR_DIM     "\x1b[2m"
#define RL_IGNORE_START "\001"
#define RL_IGNORE_END   "\002"

// Job Yapısı
typedef struct {
    pid_t pid;
    char cmd[MAX_LINE];
    int active;
} Job;

char project_root[MAX_LINE];
char *history_arr[MAX_HISTORY];
int history_count = 0;
Job jobs_list[MAX_JOBS];
Job completed_jobs[MAX_JOBS];
int completed_jobs_count = 0;
pthread_mutex_t jobs_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile sig_atomic_t child_changed = 0;

// Fonksiyon prototipleri
char *read_line(char *prompt);
char **parse_line(char *line, int *background);
int execute_command(char **args, int background, char *raw_line);
void execute_pipeline(char **args_left, char **args_right, int background, char *raw_line);
void sigchld_handler(int sig);
void show_help(void);
void print_banner(void);
void add_to_history_arr(char *line);
void show_history(void);
void clear_history_arr(void);
void show_log(int limit);
void clear_log(void);
void show_command_help(char *command);
void log_command(const char *cmd, double duration, int background, const char *status);
void add_job(pid_t pid, char *cmd);
void remove_job(pid_t pid);
void show_jobs(void);
void reap_background_jobs(void);
int get_job_by_number(int job_no, pid_t *pid);
int setup_redirection(char **args);

int main(void) {
    char *line;
    char **args;
    int status;
    int background;
    char cwd[MAX_LINE];
    char prompt_str[MAX_LINE * 2];

    mkdir("base", 0777);
    if (chdir("base") != 0) {
        perror("shell: base dizinine girilemedi");
        return EXIT_FAILURE;
    }
    getcwd(project_root, sizeof(project_root));

    // Job listesini temizle
    for (int i = 0; i < MAX_JOBS; i++) jobs_list[i].active = 0;

    struct sigaction sa;
    sa.sa_handler = &sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    print_banner();

    do {
        reap_background_jobs();

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            char *relative_path = cwd + strlen(project_root);
            snprintf(prompt_str, sizeof(prompt_str),
                     "\n" RL_IGNORE_START "%s" RL_IGNORE_END "┌─["
                     RL_IGNORE_START "%s" RL_IGNORE_END "MY-SHELL"
                     RL_IGNORE_START "%s" RL_IGNORE_END "] "
                     RL_IGNORE_START "%s" RL_IGNORE_END "/base%s"
                     RL_IGNORE_START "%s" RL_IGNORE_END "\n└─$ "
                     RL_IGNORE_START "%s" RL_IGNORE_END,
                     COLOR_DIM, COLOR_CYAN, COLOR_DIM,
                     COLOR_ORANGE, relative_path, COLOR_DIM, COLOR_RESET);
        }

        line = read_line(prompt_str);
        if (line == NULL) break;
        reap_background_jobs();
        
        if (strlen(line) > 0) {
            add_to_history_arr(line);
            add_history(line);
        }

        char raw_line[MAX_LINE];
        strcpy(raw_line, line);

        background = 0;
        args = parse_line(line, &background);

        if (args[0] == NULL) {
            free(line); free(args);
            status = 1; continue;
        }

        int pipe_pos = -1;
        for (int i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], "|") == 0) { pipe_pos = i; break; }
        }

        if (pipe_pos != -1) {
            int pipe_error = pipe_pos == 0 || args[pipe_pos + 1] == NULL || strcmp(args[pipe_pos + 1], "|") == 0;
            for (int i = pipe_pos + 1; args[i] != NULL; i++) {
                if (strcmp(args[i], "|") == 0) pipe_error = 1;
            }

            if (pipe_error) {
                printf("%sshell: pipe kullanimi hatali. Ornek: komut1 | komut2%s\n", COLOR_RED, COLOR_RESET);
                log_command(raw_line, 0, 0, "Hata");
                free(line);
                free(args);
                status = 1;
                continue;
            }
            args[pipe_pos] = NULL;
            execute_pipeline(args, &args[pipe_pos + 1], background, raw_line);
            status = 1;
        } else {
            status = execute_command(args, background, raw_line);
        }

        free(line);
        free(args);
    } while (status);

    return EXIT_SUCCESS;
}

// Job Yönetimi
void add_job(pid_t pid, char *cmd) {
    pthread_mutex_lock(&jobs_mutex);
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs_list[i].active) {
            jobs_list[i].pid = pid;
            strncpy(jobs_list[i].cmd, cmd, MAX_LINE - 1);
            jobs_list[i].cmd[MAX_LINE - 1] = '\0';
            jobs_list[i].active = 1;
            pthread_mutex_unlock(&jobs_mutex);
            return;
        }
    }
    pthread_mutex_unlock(&jobs_mutex);
}

void remove_job(pid_t pid) {
    pthread_mutex_lock(&jobs_mutex);
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs_list[i].active && jobs_list[i].pid == pid) {
            if (completed_jobs_count == MAX_JOBS) {
                for (int j = 0; j < MAX_JOBS - 1; j++) completed_jobs[j] = completed_jobs[j + 1];
                completed_jobs_count--;
            }
            completed_jobs[completed_jobs_count].pid = jobs_list[i].pid;
            strncpy(completed_jobs[completed_jobs_count].cmd, jobs_list[i].cmd, MAX_LINE - 1);
            completed_jobs[completed_jobs_count].cmd[MAX_LINE - 1] = '\0';
            completed_jobs[completed_jobs_count].active = 0;
            completed_jobs_count++;
            jobs_list[i].active = 0;
            pthread_mutex_unlock(&jobs_mutex);
            return;
        }
    }
    pthread_mutex_unlock(&jobs_mutex);
}

void show_jobs(void) {
    printf("\n%s%sAktif Arka Plan Surecleri%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%-5s %-10s %s%s\n", COLOR_DIM, "No", "PID", "Komut", COLOR_RESET);
    printf("%s----------------------------------------%s\n", COLOR_DIM, COLOR_RESET);
    int count = 0;
    pthread_mutex_lock(&jobs_mutex);
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs_list[i].active) {
            printf("%s%-5d%s %-10d %s\n", COLOR_GREEN, ++count, COLOR_RESET, jobs_list[i].pid, jobs_list[i].cmd);
        }
    }
    if (count == 0) printf("%sAktif arka plan süreci bulunamadı.%s\n", COLOR_DIM, COLOR_RESET);

    printf("\n%s%sGecmis Arka Plan Surecleri%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s%-5s %-10s %s%s\n", COLOR_DIM, "No", "PID", "Komut", COLOR_RESET);
    printf("%s----------------------------------------%s\n", COLOR_DIM, COLOR_RESET);
    if (completed_jobs_count == 0) {
        printf("%sGecmis arka plan süreci bulunamadı.%s\n", COLOR_DIM, COLOR_RESET);
    } else {
        for (int i = 0; i < completed_jobs_count; i++) {
            printf("%s%-5d%s %-10d %s\n", COLOR_ORANGE, i + 1, COLOR_RESET,
                   completed_jobs[i].pid, completed_jobs[i].cmd);
        }
    }
    pthread_mutex_unlock(&jobs_mutex);
    printf("\n");
}

int get_job_by_number(int job_no, pid_t *pid) {
    int count = 0;

    pthread_mutex_lock(&jobs_mutex);
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs_list[i].active) {
            count++;
            if (count == job_no) {
                *pid = jobs_list[i].pid;
                pthread_mutex_unlock(&jobs_mutex);
                return 1;
            }
        }
    }
    pthread_mutex_unlock(&jobs_mutex);
    return 0;
}

void sigchld_handler(int sig) {
    (void)sig;
    child_changed = 1;
}

void reap_background_jobs(void) {
    if (!child_changed) return;

    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        remove_job(pid);
    }
    child_changed = 0;
}

void log_command(const char *cmd, double duration, int background, const char *status) {
    FILE *f = fopen(LOG_FILE, "a");
    if (!f) return;
    time_t now = time(NULL);
    char *t_str = ctime(&now);
    t_str[strlen(t_str)-1] = '\0';
    fprintf(f, "[%s] CMD: %-20s | Sure: %8.4f ms | Tip: %-10s | Durum: %s\n",
            t_str, cmd, duration, background ? "Arka Plan" : "On Plan", status);
    fclose(f);
}

int setup_redirection(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        int fd;

        if (strcmp(args[i], ">") == 0 || strcmp(args[i], "<") == 0) {
            if (args[i + 1] == NULL) {
                fprintf(stderr, "shell: yonlendirme icin dosya adi gerekli\n");
                return -1;
            }

            if (strcmp(args[i], ">") == 0) {
                fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    perror("shell: output redirection error");
                    return -1;
                }
                if (dup2(fd, STDOUT_FILENO) < 0) {
                    perror("shell: dup2 output error");
                    close(fd);
                    return -1;
                }
            } else {
                fd = open(args[i + 1], O_RDONLY);
                if (fd < 0) {
                    perror("shell: input redirection error");
                    return -1;
                }
                if (dup2(fd, STDIN_FILENO) < 0) {
                    perror("shell: dup2 input error");
                    close(fd);
                    return -1;
                }
            }

            close(fd);
            int j = i;
            while (args[j + 2] != NULL) {
                args[j] = args[j + 2];
                j++;
            }
            args[j] = NULL;
            i--;
        }
    }
    return 0;
}

void execute_pipeline(char **args_left, char **args_right, int background, char *raw_line) {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    int pipefd[2];
    pid_t p1, p2;
    if (pipe(pipefd) < 0) { perror("shell: pipe error"); return; }
    p1 = fork();
    if (p1 == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]); close(pipefd[1]);
        if (setup_redirection(args_left) != 0) exit(EXIT_FAILURE);
        execvp(args_left[0], args_left);
        perror("shell: pipe left execution error");
        exit(EXIT_FAILURE);
    }
    p2 = fork();
    if (p2 == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]); close(pipefd[0]);
        if (setup_redirection(args_right) != 0) exit(EXIT_FAILURE);
        execvp(args_right[0], args_right);
        perror("shell: pipe right execution error");
        exit(EXIT_FAILURE);
    }
    close(pipefd[0]); close(pipefd[1]);
        if (!background) {
            int status1, status2;
            waitpid(p1, &status1, 0);
            waitpid(p2, &status2, 0);
            gettimeofday(&end, NULL);
            double time_spent = (double)(end.tv_usec - start.tv_usec) / 1000 + (double)(end.tv_sec - start.tv_sec) * 1000;
            log_command(raw_line, time_spent, 0,
                        (WIFEXITED(status1) && WEXITSTATUS(status1) == 0 &&
                         WIFEXITED(status2) && WEXITSTATUS(status2) == 0) ? "Tamamlandi" : "Hata");
        } else {
        add_job(p1, raw_line); // Pipe süreçlerinden birini takip etsek yeterli
        printf("%s[background pipe]%s PID: %d, %d\n", COLOR_GREEN, COLOR_RESET, p1, p2);
        log_command(raw_line, 0, 1, "Baslatildi");
    }
}

int execute_command(char **args, int background, char *raw_line) {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    if (strcmp(args[0], "exit") == 0) {
        log_command(raw_line, 0, 0, "Tamamlandi");
        return 0;
    }
    if (strcmp(args[0], "help") == 0) {
        if (args[1] != NULL) show_command_help(args[1]);
        else show_help();
        log_command(raw_line, 0, 0, "Tamamlandi");
        return 1;
    }
    if (strcmp(args[0], "history") == 0) {
        if (args[1] != NULL && strcmp(args[1], "clear") == 0) {
            clear_history_arr();
            printf("%sKomut gecmisi temizlendi.%s\n", COLOR_GREEN, COLOR_RESET);
        } else {
            show_history();
        }
        log_command(raw_line, 0, 0, "Tamamlandi");
        return 1;
    }
    if (strcmp(args[0], "jobs") == 0) { show_jobs(); log_command(raw_line, 0, 0, "Tamamlandi"); return 1; }
    if (strcmp(args[0], "killjob") == 0) {
        pid_t job_pid;
        int job_no;

        if (args[1] == NULL) {
            printf("%sshell: kullanim: killjob <no>%s\n", COLOR_RED, COLOR_RESET);
            log_command(raw_line, 0, 0, "Hata");
            return 1;
        }

        job_no = atoi(args[1]);
        if (job_no <= 0 || !get_job_by_number(job_no, &job_pid)) {
            printf("%sshell: aktif job bulunamadi.%s\n", COLOR_RED, COLOR_RESET);
            log_command(raw_line, 0, 0, "Hata");
            return 1;
        }

        if (kill(job_pid, SIGTERM) != 0) {
            perror("shell: killjob error");
            log_command(raw_line, 0, 0, "Hata");
            return 1;
        }
        waitpid(job_pid, NULL, 0);
        remove_job(job_pid);
        printf("%sJob sonlandirildi.%s PID: %d\n", COLOR_GREEN, COLOR_RESET, job_pid);
        log_command(raw_line, 0, 0, "Tamamlandi");
        return 1;
    }
    if (strcmp(args[0], "fg") == 0) {
        pid_t job_pid;
        int job_no;
        int child_status;

        if (args[1] == NULL) {
            printf("%sshell: kullanim: fg <no>%s\n", COLOR_RED, COLOR_RESET);
            log_command(raw_line, 0, 0, "Hata");
            return 1;
        }

        job_no = atoi(args[1]);
        if (job_no <= 0 || !get_job_by_number(job_no, &job_pid)) {
            printf("%sshell: aktif job bulunamadi.%s\n", COLOR_RED, COLOR_RESET);
            log_command(raw_line, 0, 0, "Hata");
            return 1;
        }

        printf("%sForeground job bekleniyor.%s PID: %d\n", COLOR_GREEN, COLOR_RESET, job_pid);
        waitpid(job_pid, &child_status, 0);
        remove_job(job_pid);
        log_command(raw_line, 0, 0,
                    (WIFEXITED(child_status) && WEXITSTATUS(child_status) == 0) ? "Tamamlandi" : "Hata");
        return 1;
    }
    if (strcmp(args[0], "log") == 0) {
        if (args[1] != NULL && strcmp(args[1], "clear") == 0) {
            clear_log();
            printf("%sLog kayitlari temizlendi.%s\n", COLOR_GREEN, COLOR_RESET);
        } else if (args[1] != NULL) {
            int limit = atoi(args[1]);
            if (limit <= 0) {
                printf("%sshell: kullanim: log veya log <sayi> veya log clear%s\n", COLOR_RED, COLOR_RESET);
                log_command(raw_line, 0, 0, "Hata");
                return 1;
            }
            show_log(limit);
        } else {
            show_log(20);
        }
        log_command(raw_line, 0, 0, "Tamamlandi");
        return 1;
    }
    if (strcmp(args[0], "clear") == 0) { printf("\033[H\033[J"); log_command(raw_line, 0, 0, "Tamamlandi"); return 1; }
    if (strcmp(args[0], "pwd") == 0) {
        char current_dir[MAX_LINE];
        if (getcwd(current_dir, sizeof(current_dir)) != NULL) {
            printf("%s\n", current_dir);
            log_command(raw_line, 0, 0, "Tamamlandi");
        } else {
            perror("shell: pwd error");
            log_command(raw_line, 0, 0, "Hata");
        }
        return 1;
    }
    
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) chdir(project_root);
        else {
            if (chdir(args[1]) == 0) {
                char current_dir[MAX_LINE];
                getcwd(current_dir, sizeof(current_dir));
                if (strncmp(current_dir, project_root, strlen(project_root)) != 0) {
                    printf("%sshell: Erişim engellendi!%s\n", COLOR_RED, COLOR_RESET);
                    chdir(project_root);
                }
            } else perror("shell: cd error");
        }
        log_command(raw_line, 0, 0, "Tamamlandi");
        return 1;
    }
    pid_t pid = fork();
    if (pid == 0) {
        if (setup_redirection(args) != 0) exit(EXIT_FAILURE);
        execvp(args[0], args);
        perror("shell: execution error");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        if (!background) {
            int child_status;
            waitpid(pid, &child_status, 0);
            gettimeofday(&end, NULL);
            double time_spent = (double)(end.tv_usec - start.tv_usec) / 1000 + (double)(end.tv_sec - start.tv_sec) * 1000;
            log_command(raw_line, time_spent, 0,
                        (WIFEXITED(child_status) && WEXITSTATUS(child_status) == 0) ? "Tamamlandi" : "Hata");
        } else {
            add_job(pid, raw_line);
            printf("%s[background]%s PID: %d\n", COLOR_GREEN, COLOR_RESET, pid);
            log_command(raw_line, 0, 1, "Baslatildi");
        }
    } else perror("shell: fork error");
    return 1;
}

void add_to_history_arr(char *line) {
    char *dup = strdup(line);
    if (history_count < MAX_HISTORY) history_arr[history_count++] = dup;
    else {
        free(history_arr[0]);
        for (int i = 0; i < MAX_HISTORY - 1; i++) history_arr[i] = history_arr[i+1];
        history_arr[MAX_HISTORY - 1] = dup;
    }
}

void show_history(void) {
    printf("\n%s%sKomut Gecmisi%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s----------------------------------------%s\n", COLOR_DIM, COLOR_RESET);
    for (int i = 0; i < history_count; i++) {
        printf("%s%2d%s  %s\n", COLOR_ORANGE, i + 1, COLOR_RESET, history_arr[i]);
    }
    printf("\n");
}

void clear_history_arr(void) {
    for (int i = 0; i < history_count; i++) {
        free(history_arr[i]);
        history_arr[i] = NULL;
    }
    history_count = 0;
    clear_history();
}

void show_log(int limit) {
    FILE *f = fopen(LOG_FILE, "r");
    char line[MAX_LINE];
    char last_lines[MAX_HISTORY * 10][MAX_LINE];
    int line_count = 0;
    int start;
    int total;

    if (limit > MAX_HISTORY * 10) limit = MAX_HISTORY * 10;

    printf("\n%s%sShell Log Kayitlari%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s----------------------------------------%s\n", COLOR_DIM, COLOR_RESET);

    if (!f) {
        perror("shell: log dosyasi acilamadi");
        printf("\n");
        return;
    }

    while (fgets(line, sizeof(line), f) != NULL) {
        strncpy(last_lines[line_count % limit], line, MAX_LINE - 1);
        last_lines[line_count % limit][MAX_LINE - 1] = '\0';
        line_count++;
    }

    total = line_count < limit ? line_count : limit;
    start = line_count > limit ? line_count % limit : 0;

    for (int i = 0; i < total; i++) {
        printf("%s", last_lines[(start + i) % limit]);
    }

    fclose(f);
    printf("\n");
}

void clear_log(void) {
    FILE *f = fopen(LOG_FILE, "w");
    if (!f) {
        perror("shell: log dosyasi temizlenemedi");
        return;
    }
    fclose(f);
}

void show_command_help(char *command) {
    printf("\n%s%sKomut Yardimi%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s----------------------------------------%s\n", COLOR_DIM, COLOR_RESET);

    if (strcmp(command, "pipe") == 0 || strcmp(command, "|") == 0) {
        printf("  %sKullanim:%s komut1 | komut2\n", COLOR_ORANGE, COLOR_RESET);
        printf("  Sol komutun ciktisini sag komuta aktarir. Ornek: ls | grep txt\n");
    } else if (strcmp(command, "jobs") == 0) {
        printf("  %sKullanim:%s jobs\n", COLOR_ORANGE, COLOR_RESET);
        printf("  Aktif ve gecmis arka plan sureclerini listeler.\n");
    } else if (strcmp(command, "killjob") == 0) {
        printf("  %sKullanim:%s killjob <no>\n", COLOR_ORANGE, COLOR_RESET);
        printf("  jobs listesindeki aktif arka plan surecini sonlandirir.\n");
    } else if (strcmp(command, "fg") == 0) {
        printf("  %sKullanim:%s fg <no>\n", COLOR_ORANGE, COLOR_RESET);
        printf("  jobs listesindeki aktif sureci foreground olarak bekler.\n");
    } else if (strcmp(command, "history") == 0) {
        printf("  %sKullanim:%s history veya history clear\n", COLOR_ORANGE, COLOR_RESET);
        printf("  Son 10 komutu gosterir veya komut gecmisini temizler.\n");
    } else if (strcmp(command, "log") == 0) {
        printf("  %sKullanim:%s log, log <sayi>, log clear\n", COLOR_ORANGE, COLOR_RESET);
        printf("  Log kayitlarini gosterir veya log dosyasini temizler.\n");
    } else if (strcmp(command, "redirect") == 0 || strcmp(command, ">") == 0 || strcmp(command, "<") == 0) {
        printf("  %sKullanim:%s komut > dosya veya komut < dosya\n", COLOR_ORANGE, COLOR_RESET);
        printf("  Komut ciktisini dosyaya yazar veya girdiyi dosyadan alir.\n");
    } else if (strcmp(command, "pwd") == 0) {
        printf("  %sKullanim:%s pwd\n", COLOR_ORANGE, COLOR_RESET);
        printf("  Bulunulan dizini gosterir.\n");
    } else {
        printf("  Bu komut icin ozel yardim yok. Genel liste icin help yazin.\n");
    }
    printf("\n");
}

void print_banner(void) {
    printf("\n");
    printf("%s╔══════════════════════════════════════╗%s\n", COLOR_BLUE, COLOR_RESET);
    printf("%s║%s  %s%-36s%s%s║%s\n",
           COLOR_BLUE, COLOR_RESET, COLOR_BOLD, "Mini Unix Shell", COLOR_RESET, COLOR_BLUE, COLOR_RESET);
    printf("%s║%s  %-36s%s║%s\n",
           COLOR_BLUE, COLOR_DIM, "help: komutlar   exit: cikis", COLOR_BLUE, COLOR_RESET);
    printf("%s╚══════════════════════════════════════╝%s\n", COLOR_BLUE, COLOR_RESET);
    show_help();
}

void show_help(void) {
    printf("\n%s%sKomut Listesi%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("%s--------------------------------------------------------------------------%s\n", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %s%s%s\n", COLOR_ORANGE, "Komut", COLOR_RESET, "Aciklama", COLOR_DIM, "Ornek", COLOR_RESET);
    printf("%s--------------------------------------------------------------------------%s\n", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %scd proje%s\n", COLOR_ORANGE, "cd <dizin>", COLOR_RESET, "Dizin degistirir", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %spwd%s\n", COLOR_ORANGE, "pwd", COLOR_RESET, "Bulunulan dizini gosterir", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %smkdir test%s\n", COLOR_ORANGE, "mkdir <dizin>", COLOR_RESET, "Yeni dizin olusturur", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %srmdir test%s\n", COLOR_ORANGE, "rmdir <dizin>", COLOR_RESET, "Bos dizini siler", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %sls%s\n", COLOR_ORANGE, "ls", COLOR_RESET, "Dizin icerigini listeler", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %stouch not.txt%s\n", COLOR_ORANGE, "touch <dosya>", COLOR_RESET, "Yeni dosya olusturur", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %snano not.txt%s\n", COLOR_ORANGE, "nano <dosya>", COLOR_RESET, "Dosyayi editorle acar", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %srm not.txt%s\n", COLOR_ORANGE, "rm <dosya>", COLOR_RESET, "Dosya siler", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %scat not.txt%s\n", COLOR_ORANGE, "cat <dosya>", COLOR_RESET, "Dosya icerigini gosterir", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %shistory%s\n", COLOR_ORANGE, "history", COLOR_RESET, "Son 10 komutu gosterir", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %shistory clear%s\n", COLOR_ORANGE, "history clear", COLOR_RESET, "Komut gecmisini temizler", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %slog%s\n", COLOR_ORANGE, "log", COLOR_RESET, "Log kayitlarini gosterir", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %slog 5%s\n", COLOR_ORANGE, "log <sayi>", COLOR_RESET, "Son N log kaydini gosterir", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %slog clear%s\n", COLOR_ORANGE, "log clear", COLOR_RESET, "Log dosyasini temizler", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %sjobs%s\n", COLOR_ORANGE, "jobs", COLOR_RESET, "Arka plan sureclerini gosterir", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %skilljob 1%s\n", COLOR_ORANGE, "killjob <no>", COLOR_RESET, "Arka plan surecini sonlandirir", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %sfg 1%s\n", COLOR_ORANGE, "fg <no>", COLOR_RESET, "Arka plan surecini bekler", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %sclear%s\n", COLOR_ORANGE, "clear", COLOR_RESET, "Ekrani temizler", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %shelp, help pipe%s\n", COLOR_ORANGE, "help [komut]", COLOR_RESET, "Yardim menusu", COLOR_DIM, COLOR_RESET);
    printf("  %s%-18s%s %-30s %sexit%s\n", COLOR_ORANGE, "exit", COLOR_RESET, "Kabuktan cikis", COLOR_DIM, COLOR_RESET);
    printf("\n%sPipe kullanimi:%s komut1 | komut2  %sOrnek:%s ls | grep txt\n",
           COLOR_DIM, COLOR_RESET, COLOR_DIM, COLOR_RESET);
    printf("%sYonlendirme:%s komut > dosya veya komut < dosya\n",
           COLOR_DIM, COLOR_RESET);
}

char *read_line(char *prompt) {
    return readline(prompt);
}

char **parse_line(char *line, int *background) {
    int bufsize = MAX_ARGS;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;
    token = strtok(line, DELIM);
    while (token != NULL) {
        tokens[position++] = token;
        if (position >= bufsize) {
            bufsize += MAX_ARGS;
            tokens = realloc(tokens, bufsize * sizeof(char *));
        }
        token = strtok(NULL, DELIM);
    }
    tokens[position] = NULL;
    if (position > 0 && strcmp(tokens[position - 1], "&") == 0) {
        *background = 1;
        tokens[position - 1] = NULL;
    }
    return tokens;
}
