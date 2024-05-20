data BTree = EmptyBTree
           | BTree Int BTree BTree
           deriving (Show, Eq)

bstAdd :: Int -> BTree -> BTree
bstAdd x EmptyBTree = BTree x EmptyBTree EmptyBTree
bstAdd x (BTree val left right)
  | x >= val = BTree val left (bstAdd x right)
  | otherwise = BTree val (bstAdd x left) right

bstFromList :: [Int] -> BTree
bstFromList [] = EmptyBTree
bstFromList (x:xs) = bstAdd x (bstFromList xs)

bstContains :: Int -> BTree -> Bool
bstContains target EmptyBTree = False
bstContains target (BTree val left right)
  | val == target = True
  | val > target = False || (bstContains target left)
  | otherwise = False || (bstContains target right)

btHeight :: BTree -> Int
btHeight EmptyBTree = 0
btHeight (BTree _ left right) = 1 + (max (btHeight left) (btHeight right))

btNumberOfNodes :: BTree -> Int
btNumberOfNodes EmptyBTree = 0
btNumberOfNodes (BTree _ left right) = 1 + (btNumberOfNodes left) + (btNumberOfNodes right)

-- Preorder Traversal
--   - Visit the root, visit all the nodes in he root's left subtree, and
--     visiting all the nodes in the root's right subtree.
btPreorder :: BTree -> [Int]
btPreorder EmptyBTree = []
btPreorder (BTree val left right) = [val] ++ (btPreorder left) ++ (btPreorder right)

-- Inorder Traversal
--   - Visit all the nodes in the root's left subtree, visit the root, and
--     visit all the nodes in the root's right subtree.
btInorder :: BTree -> [Int]
btInorder EmptyBTree = []
btInorder (BTree val left right) = (btInorder left) ++ [val] ++ (btInorder right)

-- Postorder Traversal
--   - Visit all the nodes in the root's left subtee, visit all the nodes in the
--     root's right subtree, and visit the root
btPostorder :: BTree -> [Int]
btPostorder EmptyBTree = []
btPostorder (BTree val left right) = (btPostorder left) ++ (btPostorder right) ++ [val]

bstMax :: BTree -> Int
bstMax EmptyBTree = error "empty tree"
bstMax (BTree val _ right) =
  if right == EmptyBTree
  then val
  else bstMax right

bstMin :: BTree -> Int
bstMin EmptyBTree = error "empty tree"
bstMin (BTree val left _) =
  if left == EmptyBTree
  then val
  else bstMin left

bstRemove :: Int -> BTree -> BTree
bstRemove target EmptyBTree = EmptyBTree 
bstRemove target (BTree val left right)
  | target > val = BTree val left (bstRemove target right)
  | target < val = BTree val (bstRemove target left) right
  | otherwise = if right == EmptyBTree && left == EmptyBTree
                then EmptyBTree
                else if right == EmptyBTree && left /= EmptyBTree
                then left
                else if right /= EmptyBTree && left == EmptyBTree
                then right
                else (BTree (bstMin right) left (bstRemove (bstMin right) right))
