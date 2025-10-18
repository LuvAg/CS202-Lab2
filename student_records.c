/* student_records_ext.c
 *
 * Extended student records manager with file IO, sorting, searching,
 * update, delete, histogram, and many auxiliary functions.
 *
 * Compile: gcc -std=c11 -O2 -o student_records_ext student_records_ext.c
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_STUDENTS 200
#define NAME_LEN 64
#define SUBJECTS 5

struct Student {
    char name[NAME_LEN];
    int marks[SUBJECTS];
    float avg;
    char grade;
    int valid; /* 0 = empty slot, 1 = present */
};

/* Compute average and grade for a student */
void computeGrade(struct Student *s) {
    int sum = 0;
    for (int i = 0; i < SUBJECTS; ++i) sum += s->marks[i];
    s->avg = ((float)sum) / SUBJECTS;
    if (s->avg >= 90.0f) s->grade = 'A';
    else if (s->avg >= 80.0f) s->grade = 'B';
    else if (s->avg >= 70.0f) s->grade = 'C';
    else if (s->avg >= 60.0f) s->grade = 'D';
    else s->grade = 'F';
}

/* Initialize array */
void init_students(struct Student arr[], int n) {
    for (int i = 0; i < n; ++i) arr[i].valid = 0;
}

/* Find first empty slot */
int find_empty(struct Student arr[], int n) {
    for (int i = 0; i < n; ++i) if (!arr[i].valid) return i;
    return -1;
}

/* Add student */
int add_student(struct Student arr[], int n, const char *name, int marks[]) {
    int idx = find_empty(arr, n);
    if (idx == -1) return -1;
    strncpy(arr[idx].name, name, NAME_LEN-1);
    arr[idx].name[NAME_LEN-1] = '\0';
    for (int i = 0; i < SUBJECTS; ++i) arr[idx].marks[i] = marks[i];
    computeGrade(&arr[idx]);
    arr[idx].valid = 1;
    return idx;
}

/* Delete student by exact name */
int delete_student(struct Student arr[], int n, const char *name) {
    for (int i = 0; i < n; ++i) {
        if (arr[i].valid && strcmp(arr[i].name, name) == 0) {
            arr[i].valid = 0;
            return 1;
        }
    }
    return 0;
}

/* Find student by name (exact) */
int find_student_exact(struct Student arr[], int n, const char *name) {
    for (int i = 0; i < n; ++i) if (arr[i].valid && strcmp(arr[i].name, name) == 0) return i;
    return -1;
}

/* Find students by partial match (case-insensitive) */
void to_lower(const char *src, char *dst) {
    for (; *src; ++src, ++dst) *dst = (char)tolower((unsigned char)*src);
    *dst = '\0';
}

void find_students_partial(struct Student arr[], int n, const char *sub) {
    char ssub[NAME_LEN];
    to_lower(sub, ssub);
    int found = 0;
    for (int i = 0; i < n; ++i) {
        if (!arr[i].valid) continue;
        char lname[NAME_LEN];
        to_lower(arr[i].name, lname);
        if (strstr(lname, ssub)) {
            printf("Found: %s | Avg: %.2f | Grade: %c\n", arr[i].name, arr[i].avg, arr[i].grade);
            found++;
        }
    }
    if (!found) printf("No matches for '%s'\n", sub);
}

/* Update marks for a student */
int update_marks(struct Student arr[], int n, const char *name, int marks[]) {
    int idx = find_student_exact(arr, n, name);
    if (idx == -1) return -1;
    for (int i = 0; i < SUBJECTS; ++i) arr[idx].marks[i] = marks[i];
    computeGrade(&arr[idx]);
    return idx;
}

/* Display all students */
void display_all(struct Student arr[], int n) {
    printf("\n%-25s %-8s %-6s\n", "Name", "Average", "Grade");
    for (int i = 0; i < n; ++i) {
        if (!arr[i].valid) continue;
        printf("%-25s %-8.2f %-6c\n", arr[i].name, arr[i].avg, arr[i].grade);
    }
}

/* Find topper */
int find_topper(struct Student arr[], int n) {
    int idx = -1;
    float best = -1.0f;
    for (int i = 0; i < n; ++i) {
        if (!arr[i].valid) continue;
        if (arr[i].avg > best) { best = arr[i].avg; idx = i; }
    }
    return idx;
}

/* Class average */
float class_average(struct Student arr[], int n) {
    float total = 0.0f;
    int count = 0;
    for (int i = 0; i < n; ++i) {
        if (!arr[i].valid) continue;
        total += arr[i].avg;
        ++count;
    }
    return count ? total / count : 0.0f;
}

/* Sort by average (descending) using simple bubble (sufficient for lab) */
void sort_by_avg(struct Student arr[], int n) {
    for (int i = 0; i < n-1; ++i) {
        for (int j = 0; j < n-1-i; ++j) {
            if (!arr[j].valid || !arr[j+1].valid) continue;
            if (arr[j].avg < arr[j+1].avg) {
                struct Student tmp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = tmp;
            }
        }
    }
}

/* Save to CSV */
int save_to_csv(const char *fname, struct Student arr[], int n) {
    FILE *fp = fopen(fname, "w");
    if (!fp) return -1;
    fprintf(fp, "name,avg,grade,m1,m2,m3,m4,m5\n");
    for (int i = 0; i < n; ++i) {
        if (!arr[i].valid) continue;
        fprintf(fp, "\"%s\",%.2f,%c", arr[i].name, arr[i].avg, arr[i].grade);
        for (int j = 0; j < SUBJECTS; ++j) fprintf(fp, ",%d", arr[i].marks[j]);
        fprintf(fp, "\n");
    }
    fclose(fp);
    return 0;
}

/* Load from CSV (simple parser, expects format above) */
int load_from_csv(const char *fname, struct Student arr[], int n) {
    FILE *fp = fopen(fname, "r");
    if (!fp) return -1;
    char line[512];
    if (!fgets(line, sizeof(line), fp)) { fclose(fp); return -2; } /* skip header */
    init_students(arr, n);
    while (fgets(line, sizeof(line), fp)) {
        char name[NAME_LEN];
        float avg;
        char grade;
        int m[SUBJECTS];
        /* naive parse: name may be quoted */
        char *p = line;
        if (*p == '"') {
            ++p;
            char *q = strchr(p, '"');
            if (!q) continue;
            size_t len = q - p;
            if (len >= NAME_LEN) len = NAME_LEN - 1;
            strncpy(name, p, len); name[len] = '\0';
            p = q + 1;
            if (*p == ',') ++p;
        } else {
            /* read until comma */
            char *q = strchr(p, ',');
            if (!q) continue;
            size_t len = q - p;
            if (len >= NAME_LEN) len = NAME_LEN - 1;
            strncpy(name, p, len); name[len] = '\0';
            p = q + 1;
        }
        /* read rest */
        if (sscanf(p, "%f,%c,%d,%d,%d,%d,%d", &avg, &grade, &m[0], &m[1], &m[2], &m[3], &m[4]) < 7) continue;
        int idx = find_empty(arr, n);
        if (idx == -1) break;
        strncpy(arr[idx].name, name, NAME_LEN-1);
        arr[idx].name[NAME_LEN-1] = '\0';
        for (int j = 0; j < SUBJECTS; ++j) arr[idx].marks[j] = m[j];
        computeGrade(&arr[idx]);
        arr[idx].valid = 1;
    }
    fclose(fp);
    return 0;
}

/* Histogram by grade */
void grade_histogram(struct Student arr[], int n) {
    int countA = 0, countB = 0, countC = 0, countD = 0, countF = 0, total = 0;
    for (int i = 0; i < n; ++i) {
        if (!arr[i].valid) continue;
        ++total;
        switch (arr[i].grade) {
            case 'A': ++countA; break;
            case 'B': ++countB; break;
            case 'C': ++countC; break;
            case 'D': ++countD; break;
            default: ++countF; break;
        }
    }
    printf("Grade distribution (total %d):\n", total);
    printf("A: %d\nB: %d\nC: %d\nD: %d\nF: %d\n", countA, countB, countC, countD, countF);
}

/* Helper to get an integer input safely */
int get_int_input(const char *prompt) {
    int x;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &x) == 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            return x;
        } else {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            printf("Invalid. Try again.\n");
        }
    }
}

/* Read a line (word) for name */
void get_name_input(const char *prompt, char *buf, int sz) {
    printf("%s", prompt);
    if (scanf("%63s", buf) != 1) buf[0] = '\0';
}

/* Main driver */
int main(void) {
    struct Student students[MAX_STUDENTS];
    init_students(students, MAX_STUDENTS);

    printf("=== Student Records Extended ===\n");

    while (1) {
        printf("\nMenu:\n");
        printf("1. Add student\n");
        printf("2. Update marks\n");
        printf("3. Delete student\n");
        printf("4. Display all\n");
        printf("5. Find by partial name\n");
        printf("6. Sort by average\n");
        printf("7. Topper and class average\n");
        printf("8. Save to CSV\n");
        printf("9. Load from CSV (overwrites)\n");
        printf("10. Histogram (grade distribution)\n");
        printf("11. Exit\n");
        int choice = get_int_input("Choice: ");
        if (choice == 1) {
            char name[NAME_LEN];
            get_name_input("Enter name (no spaces): ", name, NAME_LEN);
            int marks[SUBJECTS];
            for (int i = 0; i < SUBJECTS; ++i) {
                char pr[64];
                snprintf(pr, sizeof(pr), "Enter marks for subject %d: ", i+1);
                marks[i] = get_int_input(pr);
                if (marks[i] < 0) marks[i] = 0;
                if (marks[i] > 100) marks[i] = 100;
            }
            int idx = add_student(students, MAX_STUDENTS, name, marks);
            if (idx >= 0) printf("Added %s at index %d\n", name, idx);
            else printf("Class is full.\n");
        } else if (choice == 2) {
            char name[NAME_LEN];
            get_name_input("Enter exact name to update: ", name, NAME_LEN);
            int idx = find_student_exact(students, MAX_STUDENTS, name);
            if (idx == -1) { printf("Not found.\n"); continue; }
            int marks[SUBJECTS];
            for (int i = 0; i < SUBJECTS; ++i) {
                char pr[64];
                snprintf(pr, sizeof(pr), "Enter new marks for subject %d: ", i+1);
                marks[i] = get_int_input(pr);
            }
            update_marks(students, MAX_STUDENTS, name, marks);
            printf("Updated.\n");
        } else if (choice == 3) {
            char name[NAME_LEN];
            get_name_input("Enter exact name to delete: ", name, NAME_LEN);
            if (delete_student(students, MAX_STUDENTS, name)) printf("Deleted.\n");
            else printf("Not found.\n");
        } else if (choice == 4) {
            display_all(students, MAX_STUDENTS);
        } else if (choice == 5) {
            char sub[NAME_LEN];
            get_name_input("Enter substring to search: ", sub, NAME_LEN);
            find_students_partial(students, MAX_STUDENTS, sub);
        } else if (choice == 6) {
            sort_by_avg(students, MAX_STUDENTS);
            printf("Sorted by average (desc).\n");
        } else if (choice == 7) {
            int idx = find_topper(students, MAX_STUDENTS);
            if (idx == -1) printf("No students.\n");
            else printf("Topper: %s Avg: %.2f Grade: %c\n", students[idx].name, students[idx].avg, students[idx].grade);
            printf("Class average: %.2f\n", class_average(students, MAX_STUDENTS));
        } else if (choice == 8) {
            char fname[128];
            get_name_input("Enter filename to save CSV: ", fname, 128);
            if (save_to_csv(fname, students, MAX_STUDENTS) == 0) printf("Saved.\n");
            else printf("Failed to save.\n");
        } else if (choice == 9) {
            char fname[128];
            get_name_input("Enter filename to load CSV: ", fname, 128);
            if (load_from_csv(fname, students, MAX_STUDENTS) == 0) printf("Loaded.\n");
            else printf("Failed to load.\n");
        } else if (choice == 10) {
            grade_histogram(students, MAX_STUDENTS);
        } else if (choice == 11) {
            printf("Exiting.\n");
            break;
        } else {
            printf("Invalid choice.\n");
        }
    }

    return 0;
}
