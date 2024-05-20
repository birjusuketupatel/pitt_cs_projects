module BTree
  (BTree(..),
  getRootData,
  getHeight,
  getNumberOfNodes,
  showBTree)
where

data BTree a = EmptyBTree
               | BTree a (BTree a) (BTree a)
               deriving (Show, Eq, Read)

getRootData :: BTree a -> a
getRootData EmptyBTree = error "empty node"
getRootData (BTree x _ _) = x

getHeight :: BTree a -> Int
getHeight EmptyBTree = 0
getHeight (BTree _ l r) = 1 + (max (getHeight l) (getHeight r))

getNumberOfNodes :: BTree a -> Int
getNumberOfNodes EmptyBTree = 0
getNumberOfNodes (BTree _ l r) = 1 + (getNumberOfNodes l) + (getNumberOfNodes r)

showBTree :: Show a => BTree a -> String
showBTree EmptyBTree = ""
showBTree x = showOffset x 0

showOffset :: Show a => BTree a -> Int -> String
showOffset EmptyBTree offset = ""
showOffset (BTree x EmptyBTree EmptyBTree) offset = "--" ++ show x
showOffset (BTree x l r) 0 = (show x)
                             ++ "\n" ++ "+" ++ (showOffset r 1)
                             ++ "\n" ++ "+" ++ (showOffset l 1)
showOffset (BTree x l r) offset = "--" ++ (show x)
                                  ++ "\n" ++ (concat $ replicate offset "   ") ++ "+" ++ (showOffset r (offset + 1))
                                  ++ "\n" ++ (concat $ replicate offset "   ") ++ "+" ++ (showOffset l (offset + 1))
