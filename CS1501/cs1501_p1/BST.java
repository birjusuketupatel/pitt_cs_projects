package cs1501_p1;

public class BST<T extends Comparable<T>> implements BST_Inter<T> {
	
	private BTNode<T> root;
	
	//generates empty tree
	public BST() {
		root = null;
	}//BST
	
	
	//iteratively places new node into the tree
	public void put(T new_val) {
		
		BTNode<T> new_node = new BTNode<T>(new_val);
		
		//checks if new_val will be first item in a new tree, sets new_node as root
		//else places new_node in tree
		if(root == null) {
			root = new_node;
			return;
		}
		
		BTNode<T> curr_node = root;
			
		//after loop, prev_node will be parent of the new_node
		BTNode<T> prev_node = null;
			
		//flag to determine whether to insert new_node at left or right of prev_node
		boolean insert_right = false;
			
		//curr_node travels down the tree until it reaches the end of a branch
		//the leaf curr_node reaches will be the new parent of new_node
		//this leaf is stored in prev_node
		while(curr_node != null) {
				
			//prev_node trails curr_node
			prev_node = curr_node;
				
			if(go_right(new_val, curr_node)) {
				curr_node = curr_node.getRight();
					
				//trips flag if this is the last comparison
				//tells to place new_node to right of the last node
				if(curr_node == null) {
					insert_right = true;
				}
			}
			else if(go_left(new_val, curr_node)) {
				curr_node = curr_node.getLeft();
			}
			else if(equal_to(new_val, curr_node)){
				return;
			}
		}
			
		//inserts new_node to the left or right of the prev_node
		//depending on the value of the flag
		if(insert_right) {
			prev_node.setRight(new_node);
		}
		else {
			prev_node.setLeft(new_node);
		}	
	}//put
	
	
	//traverses BST iteratively, checks if input is present
	public boolean contains(T target) {
		
		BTNode<T> curr_node = root;
		boolean found = false;
		
		//travels through tree unless curr_node reaches a leaf or target is found
		//if curr_node reaches leaf, target is not in tree
		while(curr_node != null && found == false) {
			
			if(go_right(target, curr_node)) {
				curr_node = curr_node.getRight();
			}
			else if(go_left(target, curr_node)) {
				curr_node = curr_node.getLeft();
			}
			else if(equal_to(target, curr_node)){
				found = true;
			}
		}
		
		return found;
	}//contains
	
	
	//wrapper function for recursive implementation of delete
	public void delete(T target) {
		root = delete_rec(root, target);
		
	}//delete
	
	
	//recursively traverses the BST
	//finds target to be deleted
	//if target has no children or one child, cuts target from the tree
	//if target has two children, replaces target with smallest key to the right of target (swap value)
	//then deletes the swap value using recursive method
	private BTNode<T> delete_rec(BTNode<T> curr_node, T target) {
		if(curr_node == null) {
			return curr_node;
		}
		else if(go_right(target, curr_node)) {
			curr_node.setRight(delete_rec(curr_node.getRight(), target));
		}
		else if(go_left(target, curr_node)) {
			curr_node.setLeft(delete_rec(curr_node.getLeft(), target));
		}
		else if(equal_to(target, curr_node)) {
			//case 1: target node is a leaf
			//make above node's new child null
			if(is_leaf(curr_node)) {
				curr_node = null;
			}
			//case 2: target node has only 1 child
			//make above node's new child target node's child
			else if(curr_node.getLeft() != null && curr_node.getRight() == null) {
				curr_node = curr_node.getLeft();
			}
			else if(curr_node.getLeft() == null && curr_node.getRight() != null) {
				curr_node = curr_node.getRight();
			}
			//case 3: target node has 2 children
			//replaces target with smallest value on the right (swap value)
			//recursively deletes swap value
			else if(curr_node.getLeft() != null && curr_node.getRight() != null) {
				//finds smallest value to right of curr_node
				BTNode<T> swap = find_swap(curr_node);
				
				//sets curr_node value to value found
				curr_node.setKey(swap.getKey());
				
				//deletes swapped value from tree
				curr_node.setRight(delete_rec(curr_node.getRight(), swap.getKey()));
			}
			
		}
		
		return curr_node;
	}//delete_rec
	
	//finds smallest value on the right of the node to be deleted
	//returns this node
	private BTNode<T> find_swap(BTNode<T> to_delete){
		//goes once to the right
		BTNode<T> curr_swap = to_delete.getRight();
		
		//goes as far left as possible
		while(curr_swap.getLeft() != null) {
			curr_swap = curr_swap.getLeft();
		}
		
		//curr_swap is the next largest value after to_delete
		return curr_swap;
	}//find_swap
	
	
	//wrapper function for recursive implementation of in order traversal
	public String inOrderTraversal() {
		String path = "";
		path = rec_in_order(root, path);
		
		//removes last colon from path
		if(path.length() > 1) {
			path = path.substring(0, path.length() - 1);
		}
		
		return path;
	}//inOrderTraversal
	
	
	//recursively traverses tree in order
	private String rec_in_order(BTNode<T> curr_node, String path) {
		if(curr_node == null) {
			return path;
		}
		
		path = rec_in_order(curr_node.getLeft(), path);
		path = path + curr_node.getKey() + ":";
		path = rec_in_order(curr_node.getRight(), path);
		
		return path;
	}//rec_in_order
	
	
	//wrapper function calls recursive function to calculate height
	public int height() {
		return rec_height(root);
	}//height
	
	
	//recursively traverses all branches and stores the height of tallest branch
	private int rec_height(BTNode<T> curr_node) {
		if(curr_node == null) {
			return 0;
		}
		else {
			int left_height = rec_height(curr_node.getLeft());
			int right_heigth = rec_height(curr_node.getRight());
			
			if(left_height > right_heigth) {
				return left_height + 1;
			}
			else {
				return right_heigth + 1;
			}
		}
	}//rec_height
	
	
	//wrapper function calls recursive function
	public boolean isBalanced() {
		return rec_balanced(root);
	}//isBalanced
	
	
	//recursively traverses all branches and tests if subtrees differ in height by more than 1
	private boolean rec_balanced(BTNode<T> curr_node) {
		if(curr_node == null) {
			return true;
		}
		
		BTNode<T> left = curr_node.getLeft();
		BTNode<T> right = curr_node.getRight();
		
		int left_max_height = this.rec_height(left);
		int right_max_height = this.rec_height(right);
		
		//if left and right subtrees differ in height by more then 1, not height balanced
		//otherwise, checks if subtrees of subtrees are height balanced
		if(Math.abs(right_max_height - left_max_height) > 1) {
			return false;
		}
		else {
			return(rec_balanced(left) && rec_balanced(right));
		}
	}//rec_balanced
	
	
	//wrapper function calls recursive implementation of pre-order traversal
	public String serialize() {
		String path = "";
		
		if(root == null) {
			return path;
		}
		
		path = rec_preorder(root, root, path);
		
		//removes last comma from string
		if(path.length() > 1) {
			path = path.substring(0, path.length() - 1);
		}
		
		return path;
	}//serialize
	
	
	//recursively traverses tree in pre-order fashion
	//places appropriate labels on keys based on their properties
	private String rec_preorder(BTNode<T> curr_node, BTNode<T> prev_node, String path) {
		
		String segment = "";
		//prev_node stores node in tree before curr_node
		//case 1: if previous is not a leaf, and are at a null location, adds null to signify missing branch
		if(curr_node == null && (prev_node.getLeft() != null || prev_node.getRight() != null)) {
			segment = "X(NULL),";
		}
		//case 2: if we are past a leaf node, add nothing
		else if(curr_node == null) {
			segment = "";
		}
		//case 3: if at a root, add R() to key value
		else if(curr_node == root) {
			segment = "R(" + curr_node.getKey() + "),";
		}
		//case 4: if at leaf node, add L() to key value
		else if(is_leaf(curr_node)) {
			segment = "L(" + curr_node.getKey() + "),";
		}
		//case 5: if at an interior node, add I() to key value
		else if(curr_node.getLeft() != null || curr_node.getRight() != null) {
			segment = "I(" + curr_node.getKey() + "),";
		}
		
		//add key value to the list
		path = path + segment;
		
		if(curr_node == null) {
			return path;
		}
		
		path = rec_preorder(curr_node.getLeft(), curr_node, path);
		path = rec_preorder(curr_node.getRight(), curr_node, path);
		
		return path;
	}//rec_preorder
	
	
	//returns a copy of the BST reversed
	public BST<T> reverse() {
		BST<T> reversed = new BST<T>();
		reversed = copy();
		
		rec_reverse(reversed.root);
		
		return reversed;
	}//reverse
	
	
	//recursively reverses a BST
	//traverses BST in post-order fashion
	private void rec_reverse(BTNode<T> curr_node) {
		if(curr_node == null) {
			return;
		}
		
		//all child nodes must be reversed before current nodes can be reversed
		rec_reverse(curr_node.getLeft());
		rec_reverse(curr_node.getRight());
		
		//swap left and right child of current node
		BTNode<T> temp = curr_node.getLeft();
		curr_node.setLeft(curr_node.getRight());
		curr_node.setRight(temp);
	}//rec_reverse
	
	
	//wrapper function calls rec_copy
	public BST<T> copy() {
		BST<T> copy = new BST<T>();
		copy = rec_copy(copy, root);
		return copy;
	}//copy
	
	
	//uses recursion to make a copy of current BST
	//traverses BST in pre-order fashion
	private BST<T> rec_copy(BST<T> copy, BTNode<T> curr_node){
		if(curr_node == null) {
			return copy;
		}
		
		//put current node into new tree first
		copy.put(curr_node.getKey());
		
		//then put the rest in
		copy = rec_copy(copy, curr_node.getLeft());
		copy = rec_copy(copy, curr_node.getRight());
		
		return copy;
	}//deep_copy
	
	
	//returns true if node is a leaf
	private boolean is_leaf(BTNode<T> curr_node) {
		return (curr_node.getLeft() == null && curr_node.getRight() == null);
	}//is_leaf
	
	
	//returns key > curr_val
	private boolean go_right(T key, BTNode<T> curr_val) {
		return (key.compareTo(curr_val.getKey()) > 0);
	}//greater_than
	
	
	//returns key < curr_val
	private boolean go_left(T key, BTNode<T> curr_val) {
		return (key.compareTo(curr_val.getKey()) < 0);
	}//less_than
	
	
	//returns key == curr_val
	private boolean equal_to(T key, BTNode<T> curr_val) {
		return (key.compareTo(curr_val.getKey()) == 0);
	}//equal_to
	
	
}//BST
