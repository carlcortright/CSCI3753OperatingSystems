/*
* Search, Insert and Delete operations on a linked list without race conditions
*/
semaphore ins;
semaphore del;

/*
* Mutually Exclusive search method
*/
int Search(int i){
  wait(&del);
  int result = search(i);
  signal(&del);
}

/*
* Mutually Exclusive insert method
*/
void Insert(int i){
  wait(&ins);
  wait(&del);
  insert(i);
  signal(&del);
  signal(&ins);
}

/*
* Mutually Exclusive delete method
*/
void Delete(int i){
  wait(&del);
  delete(i);
  signal(&del);
}
