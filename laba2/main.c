#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>


int validate_nums(int amount, char* nums[]) {
    for (int j = 0; j <= amount; ++j) {
        int len_str = strlen(nums[j]);
        for (int i = 0; i < len_str; ++i) {
            if (nums[j][i] < '0' || nums[j][i] > '9') {
                return 0;
            }
        }
    }
    return 1;
}

void float_to_string(long double value, char *buffer, int len, int precision) {
    for (int i = 0; i < len; ++i) {
        buffer[i] = '\0';
    }
    if (value < 0) {
        *buffer++ = '-';
        value = -value;
    }
    int integerPart = (int)value;
    float fractionalPart = value - integerPart;

    char *intPtr = buffer;
    if (integerPart == 0) {
        *intPtr++ = '0';
    } else {
        char temp[20];
        int i = 0;
        while (integerPart > 0) {
            temp[i++] = (integerPart % 10) + '0';
            integerPart /= 10;
        }
        while (i > 0) {
            *intPtr++ = temp[--i];
        }
    }

    *intPtr++ = '.';

    for (int i = 0; i < precision; i++) {
        fractionalPart *= 10;
        int fractionalDigit = (int)fractionalPart;
        *intPtr++ = fractionalDigit + '0';
        fractionalPart -= fractionalDigit;
    }

    *intPtr++ = '%';
    *intPtr = '\n';
}

// Функция, выполняемая потоками
void* calc_score(void* arg) {
    unsigned long current  = time(NULL);
    const unsigned long a = 1664525;
    const unsigned long c = 1013904223;
    const unsigned long m = 4294967296;

    int rounds = ((int*)arg)[0];
    int exp = ((int*)arg)[1];
    int first_score = ((int*)arg)[2];
    int second_score = ((int*)arg)[3];

    int* score = malloc(sizeof(int) * 2);
    long double* result = malloc(sizeof(long double) * 3);
    result[0] = result[1] = result[2] = 0;

    for (int j = 0; j < exp; j++) {
        score[0] = first_score;
        score[1] = second_score;
        for (int i = 0; i < rounds; i++) {
            current = (current * a + c) % m;
            score[0] += current % 6 + 1;
            current = (current * a + c) % m;
            score[0] += current % 6 + 1;

            current = (current * a + c) % m;
            score[1] += current % 6 + 1;
            current = (current * a + c) % m;
            score[1] += current % 6 + 1;
        }

        // Защита доступа к результатам
        if (score[0] > score[1]) {
            result[0] += 1.0;
        } else if (score[0] < score[1]) {
            result[1] += 1.0;
        }
        result[2] += 1.0;
    }

    free(arg);
    free(score);
    return result;
}

int main(int argc, char* argv[]) {
    if (argc != 7) {
        char msg[] = "USAGE: ./a.out <total rounds> <current tour> <first_score> <second_score> <experiments> <threads>\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 1;
    }

    if (validate_nums(argc, argv)) {
        char msg[] = "ERROR: all input numbers must be integer and positive\n";

        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 2;
    }

    int k = atoi(argv[1]);
    int tour = atoi(argv[2]);
    int first_score = atoi(argv[3]);
    int second_score = atoi(argv[4]);
    int experiments = atoi(argv[5]);
    int num_threads = atoi(argv[6]);

    if (k < tour) {
        char msg[] = "ERROR: total rounds must be greater than current tour\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 3;
    }

    if ( num_threads < 1) {
        char msg[] = "ERROR: number of threads must be greater than 1\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return 4;
    }

    pthread_t experiments_threads[num_threads];
    long double total_first_prob = 0.;
    long double total_second_prob = 0.;
    long double total_exp = 0.;

    for (int i = 0; i < num_threads; ++i) {
        int* data_for_calc = malloc(sizeof(int) * 4);
        data_for_calc[0] = k - tour; // Количество бросков
        data_for_calc[1] = experiments / num_threads + (i < experiments % num_threads); // Количество экспериментов
        data_for_calc[2] = first_score; // Очки первого игрока
        data_for_calc[3] = second_score; // Очки второго игрока

        if (pthread_create(&experiments_threads[i], NULL, calc_score, data_for_calc)) {
            char msg[] = "ERROR: thread cannot be created\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            return 5;
        }
    }

    for (int i = 0; i < num_threads; ++i) {
        long double* scores;
        if (pthread_join(experiments_threads[i], (void**)&scores)) {
            char msg[] = "ERROR: thread cannot be joined\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            return 6;
        }

        // Каждый поток возвращает результаты, мы их суммируем
        total_first_prob += scores[0];
        total_second_prob += scores[1];
        total_exp += scores[2];
        free(scores); // Освобождение памяти после использования
    }
    total_first_prob = total_first_prob / total_exp * 100;
    total_second_prob = total_second_prob / total_exp * 100;

    char num[16];
    float_to_string(total_first_prob, num, 16, 2);

    char msg1[] = "Probability of the first player winnig - ";
    write(STDOUT_FILENO, msg1, sizeof(msg1) - 1);
    write(STDOUT_FILENO, num, sizeof(num) - 1);

    float_to_string(total_second_prob, num, 16, 2);

    char msg2[] = "Probability of the second player winnig - ";
    write(STDOUT_FILENO, msg2, sizeof(msg2) - 1);
    write(STDOUT_FILENO, num, sizeof(num) - 1);

    return 0;
}
