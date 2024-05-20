-- Birju Patel
-- Dr. Tan, CS1699
-- February 12, 2023

quickSort :: Ord a => [a] -> [a]
quickSort [] = []
quickSort [x] = [x]
quickSort (x:xs) =
  let l = [y | y <- xs, y < x]
      r = [y | y <- xs, y >= x]
  in quickSort l ++ [x] ++ quickSort r

mergeSort :: Ord a => [a] -> [a]
mergeSort [] = []
mergeSort [x] = [x]
mergeSort (x:xs) =
  let mid = div (length (x:xs)) 2
      l = take mid (x:xs)
      r = takeRemainder mid (x:xs)
      sorted_l = mergeSort l
      sorted_r = mergeSort r
  in merge sorted_l sorted_r

merge :: Ord a => [a] -> [a] -> [a]
-- merge [1,4] [2,5] -> [1,2,4,5]
merge x [] = x
merge [] y = y
merge (x:xs) (y:ys)
  | x > y = y : merge (x:xs) ys
  | x <= y = x : merge xs (y:ys)

takeRemainder :: Int -> [a] -> [a]
-- takeRemainder 2 [1,2,3,4,5] = [3,4,5]
takeRemainder x [] = []
takeRemainder 0 y = y
takeRemainder x (y:ys) = takeRemainder (x - 1) ys

insertionSort :: Ord a => [a] -> [a]
insertionSort [x] = [x]
insertionSort (x:xs) = insert x (insertionSort xs)

insert :: Ord a => a -> [a] -> [a]
insert x [] = [x]
insert x (y:ys)
  | x <= y = x : (y:ys)
  | otherwise = y : insert x ys

selectionSort :: Ord a => [a] -> [a]
selectionSort [] = []
selectionSort [x] = [x]
selectionSort x =
  let min = myMin x
      rem = removeFirst min x
  in min : selectionSort rem

removeFirst :: Ord a => a -> [a] -> [a]
-- removeFirst 3 [1,3,3,4] -> [1,3,4]
removeFirst target [] = []
removeFirst target (x:xs)
  | x == target = xs
  | otherwise = x : removeFirst target xs

myMin :: Ord a => [a] -> a
-- myMin [5,6,1,7] -> 1
myMin [] = error "empty list"
myMin [x] = x
myMin (x:xs)
  | x < y = x
  | x >= y = y
  where y = myMin xs

bubbleSort :: Ord a => [a] -> [a]
bubbleSort x = if (isSorted x) then x else bubbleSort (pass x)

isSorted :: Ord a => [a] -> Bool
isSorted [] = True
isSorted [x] = True
isSorted (x:xs) = (x <= head xs) && isSorted xs

pass :: Ord a => [a] -> [a]
pass [] = []
pass [x] = [x]
pass (a:b:c)
  | a > b = b : pass (a:c)
  | otherwise = a : pass (b:c)
