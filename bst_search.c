/* bst_ext.c
 *
 * Extended Binary Search Tree program with many utilities:
 * - insert (recursive), insert_iterative
 * - delete, search
 * - traversals: inorder, preorder, postorder, level-order
 * - height, node count, leaf count
 * - save/load to file, print stats
 *
 * Compile: gcc -std=c11 -O2 -o bst_ext bst_ext.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    int key;
    struct Node *left;
    struct Node *right;
};

/* Create a new node */
struct Node* newNode(int key) {
    struct Node* n = (struct Node*)malloc(sizeof(struct Node));
    if (!n) { perror("malloc"); exit(1); }
    n->key = key;
    n->left = n->right = NULL;
    return n;
}

/* Recursive insert */
struct Node* insert_recursive(struct Node* root, int key) {
    if (root == NULL) return newNode(key);
    if (key < root->key) root->left = insert_recursive(root->left, key);
    else if (key > root->key) root->right = insert_recursive(root->right, key);
    /* if equal, ignore duplicate */
    return root;
}

/* Iterative insert */
struct Node* insert_iterative(struct Node* root, int key) {
    if (root == NULL) return newNode(key);
    struct Node* cur = root;
    struct Node* parent = NULL;
    while (cur) {
        parent = cur;
        if (key < cur->key) cur = cur->left;
        else if (key > cur->key) cur = cur->right;
        else return root; /* duplicate */
    }
    if (key < parent->key) parent->left = newNode(key);
    else parent->right = newNode(key);
    return root;
}

/* Search recursively */
struct Node* search_recursive(struct Node* root, int key) {
    if (root == NULL || root->key == key) return root;
    if (key < root->key) return search_recursive(root->left, key);
    else return search_recursive(root->right, key);
}

/* Minimum value node */
struct Node* minValueNode(struct Node* node) {
    struct Node* cur = node;
    while (cur && cur->left) cur = cur->left;
    return cur;
}

/* Delete node */
struct Node* deleteNode(struct Node* root, int key) {
    if (root == NULL) return root;
    if (key < root->key) root->left = deleteNode(root->left, key);
    else if (key > root->key) root->right = deleteNode(root->right, key);
    else {
        /* Node with only one child or no child */
        if (root->left == NULL) {
            struct Node* temp = root->right;
            free(root);
            return temp;
        } else if (root->right == NULL) {
            struct Node* temp = root->left;
            free(root);
            return temp;
        }
        /* Node with two children */
        struct Node* temp = minValueNode(root->right);
        root->key = temp->key;
        root->right = deleteNode(root->right, temp->key);
    }
    return root;
}

/* Traversals */
void inorder(struct Node* root) {
    if (!root) return;
    inorder(root->left);
    printf("%d ", root->key);
    inorder(root->right);
}

void preorder(struct Node* root) {
    if (!root) return;
    printf("%d ", root->key);
    preorder(root->left);
    preorder(root->right);
}

void postorder(struct Node* root) {
    if (!root) return;
    postorder(root->left);
    postorder(root->right);
    printf("%d ", root->key);
}

/* Get height */
int height(struct Node* root) {
    if (!root) return 0;
    int lh = height(root->left);
    int rh = height(root->right);
    return (lh > rh ? lh : rh) + 1;
}

/* Count nodes */
int count_nodes(struct Node* root) {
    if (!root) return 0;
    return 1 + count_nodes(root->left) + count_nodes(root->right);
}

/* Count leaves */
int count_leaves(struct Node* root) {
    if (!root) return 0;
    if (!root->left && !root->right) return 1;
    return count_leaves(root->left) + count_leaves(root->right);
}

/* Level order traversal (BFS) using queue */
struct QueueNode {
    struct Node *treeNode;
    struct QueueNode *next;
};

void enqueue(struct QueueNode **head, struct QueueNode **tail, struct Node *tn) {
    struct QueueNode *q = (struct QueueNode*)malloc(sizeof(struct QueueNode));
    if (!q) { perror("malloc"); exit(1); }
    q->treeNode = tn;
    q->next = NULL;
    if (!*tail) { *head = *tail = q; }
    else { (*tail)->next = q; *tail = q; }
}

struct Node* dequeue(struct QueueNode **head, struct QueueNode **tail) {
    if (!*head) return NULL;
    struct QueueNode *tmp = *head;
    struct Node *tn = tmp->treeNode;
    *head = (*head)->next;
    if (!*head) *tail = NULL;
    free(tmp);
    return tn;
}

void levelOrder(struct Node* root) {
    struct QueueNode *head = NULL, *tail = NULL;
    if (!root) return;
    enqueue(&head, &tail, root);
    while (head) {
        struct Node *n = dequeue(&head, &tail);
        printf("%d ", n->key);
        if (n->left) enqueue(&head, &tail, n->left);
        if (n->right) enqueue(&head, &tail, n->right);
    }
}

/* Find predecessor (max in left subtree) */
struct Node* predecessor(struct Node* root, int key) {
    struct Node* cur = root;
    struct Node* pred = NULL;
    while (cur) {
        if (key > cur->key) {
            pred = cur;
            cur = cur->right;
        } else if (key < cur->key) {
            cur = cur->left;
        } else {
            if (cur->left) {
                pred = cur->left;
                while (pred->right) pred = pred->right;
            }
            break;
        }
    }
    return pred;
}

/* Find successor (min in right subtree) */
struct Node* successor(struct Node* root, int key) {
    struct Node* cur = root;
    struct Node* succ = NULL;
    while (cur) {
        if (key < cur->key) {
            succ = cur;
            cur = cur->left;
        } else if (key > cur->key) {
            cur = cur->right;
        } else {
            if (cur->right) {
                succ = cur->right;
                while (succ->left) succ = succ->left;
            }
            break;
        }
    }
    return succ;
}

/* Save tree to file using preorder with marker for NULL */
void save_tree_preorder(FILE *fp, struct Node* root) {
    if (root == NULL) {
        fprintf(fp, "# ");
        return;
    }
    fprintf(fp, "%d ", root->key);
    save_tree_preorder(fp, root->left);
    save_tree_preorder(fp, root->right);
}

/* Load tree from preorder with NULL markers */
struct Node* load_tree_preorder(FILE *fp) {
    char buf[64];
    if (!(fscanf(fp, "%63s", buf) == 1)) return NULL;
    if (strcmp(buf, "#") == 0) return NULL;
    int val = atoi(buf);
    struct Node* root = newNode(val);
    root->left = load_tree_preorder(fp);
    root->right = load_tree_preorder(fp);
    return root;
}

/* Free tree memory */
void free_tree(struct Node* root) {
    if (!root) return;
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}

/* Menu driver */
int main(void) {
    struct Node* root = NULL;
    int choice;
    int key;
    char fname[128];

    printf("=== Extended BST Program ===\n");

    while (1) {
        printf("\nMenu:\n");
        printf("1. Insert (recursive)\n");
        printf("2. Insert (iterative)\n");
        printf("3. Search\n");
        printf("4. Delete\n");
        printf("5. Traversals (in/pre/post/level)\n");
        printf("6. Statistics (height, nodes, leaves)\n");
        printf("7. Find predecessor & successor\n");
        printf("8. Save tree to file\n");
        printf("9. Load tree from file (overwrites current)\n");
        printf("10. Clear tree\n");
        printf("11. Exit\n");
        printf("Choice: ");
        if (scanf("%d", &choice) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            continue;
        }

        if (choice == 1) {
            printf("Enter key to insert: ");
            if (scanf("%d", &key) == 1) root = insert_recursive(root, key);
        } else if (choice == 2) {
            printf("Enter key to insert (iterative): ");
            if (scanf("%d", &key) == 1) root = insert_iterative(root, key);
        } else if (choice == 3) {
            printf("Enter key to search: ");
            if (scanf("%d", &key) == 1) {
                struct Node* found = search_recursive(root, key);
                if (found) printf("Found key %d\n", found->key);
                else printf("Key %d not found\n", key);
            }
        } else if (choice == 4) {
            printf("Enter key to delete: ");
            if (scanf("%d", &key) == 1) {
                root = deleteNode(root, key);
                printf("Deleted (if existed) %d\n", key);
            }
        } else if (choice == 5) {
            printf("Inorder: ");
            inorder(root);
            printf("\nPreorder: ");
            preorder(root);
            printf("\nPostorder: ");
            postorder(root);
            printf("\nLevel-order: ");
            levelOrder(root);
            printf("\n");
        } else if (choice == 6) {
            printf("Height: %d\n", height(root));
            printf("Nodes: %d\n", count_nodes(root));
            printf("Leaves: %d\n", count_leaves(root));
        } else if (choice == 7) {
            printf("Enter key to find pred & succ: ");
            if (scanf("%d", &key) == 1) {
                struct Node* pred = predecessor(root, key);
                struct Node* succ = successor(root, key);
                if (pred) printf("Predecessor: %d\n", pred->key); else printf("No predecessor\n");
                if (succ) printf("Successor: %d\n", succ->key); else printf("No successor\n");
            }
        } else if (choice == 8) {
            printf("Enter filename to save: ");
            if (scanf("%127s", fname) == 1) {
                FILE *fp = fopen(fname, "w");
                if (!fp) { printf("Failed to open file\n"); }
                else { save_tree_preorder(fp, root); fclose(fp); printf("Saved\n"); }
            }
        } else if (choice == 9) {
            printf("Enter filename to load: ");
            if (scanf("%127s", fname) == 1) {
                FILE *fp = fopen(fname, "r");
                if (!fp) { printf("Failed to open file\n"); }
                else {
                    free_tree(root);
                    root = load_tree_preorder(fp);
                    fclose(fp);
                    printf("Loaded tree from %s\n", fname);
                }
            }
        } else if (choice == 10) {
            free_tree(root);
            root = NULL;
            printf("Cleared tree\n");
        } else if (choice == 11) {
            printf("Exiting.\n");
            break;
        } else {
            printf("Invalid choice.\n");
        }
    }

    free_tree(root);
    return 0;
}
