-- Birju Patel
-- Dr. Tan, CS1699
-- February 5, 2023

myNull :: [a] -> Bool
-- myNull [] -> True
-- myNull [1,2,3] -> False
myNull [] = True
myNull (x:xs) = False

(+++) :: [a] -> [a] -> [a]
-- (+++) [1,2,3] [4,5] -> [1,2,3,4,5]
-- "hello " +++ " world" -> "hello world"
(+++) [] (y:ys) = y:ys
(+++) (x:xs) [] = x:xs
(+++) (x:xs) (y:ys) = x : (+++) xs (y:ys)

myHead :: [a] -> a
-- myHead [1,2,3] -> 1
myHead [] = error "empty list"
myHead (x:xs) = x

myTail :: [a] -> [a]
-- myTail [1,2,3] -> [2,3]
myTail [] = error "empty list"
myTail (x:xs) = xs

myReverse :: [a] -> [a]
-- myReverse [1,2,3] -> [3,2,1]
myReverse [] = []
myReverse (x:xs) = (myReverse xs) +++ [x]

myLast :: [a] -> a
-- myLast [1,2,3] -> 3
myLast [] = error "empty list"
myLast [x] = x
myLast (x:xs) = myLast xs

myInit :: [a] -> [a]
-- myInit [1,2,3] -> [1,2]
myInit [] = error "empty list"
myInit [x] = []
myInit (x:xs) = x : myInit(xs)

myLength :: [a] -> Int
-- myLength [1,2,3] = 3
myLength [] = 0
myLength (x:xs) = 1 + myLength xs

myTake :: Int -> [a] -> [a]
-- myTake 2 [1,2,3,4] -> [1,2]
myTake x [] = []
myTake 0 y = []
myTake x (y:ys) = y : myTake (x - 1) ys

myDrop :: Int -> [a] -> [a]
-- myDrop 2 [1,2,3,4] -> [3,4]
myDrop x [] = []
myDrop 0 y = y
myDrop x (y:ys) = myDrop (x - 1) ys

myMaximum :: Ord a => [a] -> a
-- myMaximum [1,4,2,3] -> 4
myMaximum [] = error "empty list"
myMaximum [x] = x
myMaximum (x:xs)
  | x >= y = x
  | y > x = y
  where y = myMaximum xs

myMinimum :: Ord a => [a] -> a
-- myMinimum [1,4,2,3] -> 1
myMinimum [] = error "empty list"
myMinimum [x] = x
myMinimum (x:xs)
  | x <= y = x
  | y < x = y
  where y = myMinimum xs

mySum :: Num a => [a] -> a
-- mySum [1,2,3] -> 6
mySum [] = error "empty list"
mySum [x] = x
mySum (x:xs) = x + mySum xs

myProduct :: Num a => [a] -> a
-- myProduct [1,2,3,4] -> 24
myProduct [] = error "empty list"
myProduct [x] = x
myProduct (x:xs) = x * myProduct(xs)

myElem :: Eq a => a -> [a] -> Bool
-- myElem 2 [1,2,3] -> True
-- myElem 4 [1,2,3] -> False
myElem x [] = False
myElem x (y:ys) = x == y || myElem x ys
