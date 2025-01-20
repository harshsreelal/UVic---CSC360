#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "linked_list.h"

 
Node * add_newNode(Node* head, pid_t new_pid, char * new_path){
	// Your code here
	Node* new = (Node*)malloc(sizeof(Node));
	new->pid = new_pid;
	new->path = strdup(new_path);
	new->next = NULL;

	if (head == NULL) {
		head = new;
	} else {
		Node* node = head;
		while (node->next != NULL) {
			node = node->next;
		}
		node->next = new;
	}

	printf("%d, %s\n", new->pid, new->path);
	return head; 
}


Node * deleteNode(Node* head, pid_t pid) {
	// your code here
	if (!pidExist(head, pid)) {
		printf("Process ID doesnt not exist");
		return;
	}
	Node* node1 = head;
	if (node1 == head) {
		head = node1->next;
		free(node1);
		return head;
	}
	Node* node2 = NULL;
	while (node1 != NULL) {
		if (node1->pid == pid) {
			node2->next = node1->next;
		}
		free(node1);
		return;
	}
		node2 = node1;
		node1 = node1->next;
	return node1;
} 


void printList(Node *head){
	// your code here
	int count = 0;
  	Node* node = head;
  
 	while (node != NULL) {
		count++;
		printf("%d: %s\n", node->pid, node->path);
		node = node->next;
  	}
  	printf("Total number of background processes: %d\n", count);
}

int pidExist(Node *node, pid_t pid){
	// your code here
	while (node != NULL) {
		if (node->pid == pid) {
			return true;
		}
		node = node->next;
	}
	return false;
}

