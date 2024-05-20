module HuffmanCoding
  (toHuffmanCode,
  fromHuffmanCode)
where

import BTree
import Data.List

data HBit = L | R deriving (Eq, Show, Read)

--given a word, returns its huffman tree and encoding
toHuffmanCode :: [Char] -> ([Char],[Char])
toHuffmanCode x = let tree = collapseTrees $ initTrees x
                      code = encodeWord tree x
                  in (show tree, show code)

--given a string representation of a huffman tree and an encoding, returns the decoded word
fromHuffmanCode :: [Char] -> [Char] -> [Char]
fromHuffmanCode x y = let tree = read x :: BTree (Char,Int)
                          code = read y :: [HBit]
                      in decodeWord tree tree code

--given a huffman tree and an encoding, returns the decoded word
decodeWord :: BTree (Char,Int) -> BTree (Char,Int) -> [HBit] -> [Char]
decodeWord _ (BTree x EmptyBTree EmptyBTree) [] = [fst x]
decodeWord _ _ [] = error "invalid encoding"
decodeWord _ EmptyBTree _ = error "invalid encoding"
decodeWord root (BTree x EmptyBTree EmptyBTree) xs = fst x : decodeWord root root xs
decodeWord root (BTree _ l r) (x:xs) = if x == L
                                       then decodeWord root l xs
                                       else decodeWord root r xs

--sorts huffman trees by frequency
sortTrees :: [BTree (Char,Int)] -> [BTree (Char,Int)]
sortTrees xs = sortBy (\x y -> compare (snd $ getRootData x) (snd $ getRootData y)) xs

--creates initial list of huffman trees given a string
initTrees :: [Char] -> [BTree (Char,Int)]
initTrees xs = sortTrees $ [BTree x EmptyBTree EmptyBTree | x <- charCounts xs]

--given a string creates a list of characters and their frequency in the string
charCounts :: [Char] -> [(Char,Int)]
charCounts xs = foldl (\acc x -> if elem x $ map fst acc
                                 then acc
                                 else (x, charCount x xs) : acc
                      ) [] xs

--gets number of times a character appears in a string
charCount :: Char -> [Char] -> Int
charCount c str = length $ filter (== c) str

--repeatedly combines the two smallest huffman trees in a list until one remains
--final tree must have a height > 1
collapseTrees :: [BTree (Char,Int)] -> BTree (Char,Int)
collapseTrees [] = error "empty list"
collapseTrees [x] = if (getHeight x) > 1
                    then x
                    else BTree ('\NUL',0) x EmptyBTree
collapseTrees [x,y] = combine x y
collapseTrees (x:y:xs) = collapseTrees $ sortTrees ((combine x y) : xs)

--combines two huffman trees into one
combine :: BTree (Char,Int) -> BTree (Char,Int) -> BTree (Char,Int)
combine x y = let freq = (snd $ getRootData x) + (snd $ getRootData y)
              in BTree ('\NUL',freq) x y

--given a letter and a huffman tree, encodes the letter
encodeLetter :: BTree (Char,Int) -> Char -> [HBit]
encodeLetter EmptyBTree c = error "empty tree"
encodeLetter (BTree x l r) c = if (fst x) == c
                               then []
                               else if containsLetter l c
                               then L : encodeLetter l c
                               else if containsLetter r c
                               then R : encodeLetter r c
                               else error "character not in tree"

--given a letter and a huffman tree, returns if the tree contains the letter
containsLetter :: BTree (Char,Int) -> Char -> Bool
containsLetter EmptyBTree c = False
containsLetter (BTree x l r) c = if (fst x) == c
                                 then True
                                 else (containsLetter l c) || (containsLetter r c)

--given a word and a huffman tree, encodes the word
encodeWord :: BTree (Char,Int) -> [Char] -> [HBit]
encodeWord tree x = foldl (++) [] $ map (encodeLetter tree) x
