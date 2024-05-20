-- Assignment 4

elemNum :: Eq a => a -> [a] -> Int
elemNum target ls = length (filter (== target) ls)

maxOccurs :: Ord a => [a] -> (a, Int)
maxOccurs [] = error "empty list"
maxOccurs ls = let val = maximum ls
               in (val, foldl (\acc x -> if x == val then acc + 1 else acc) 0 ls)

onlyUnique :: Eq a => [a] -> [a]
onlyUnique xs = let count y ys = length $ filter (==y) ys
                in foldr (\x acc -> if (count x xs) <= 1 then x : acc else acc) [] xs

listToSet :: Eq a => [a] -> [a]
listToSet = foldr (\x acc -> if elem x acc then acc else x : acc) []

subset :: Eq a => [a] -> [a] -> Bool
subset xs ys = foldl (\acc x -> acc && (elem x ys)) True xs

setEq :: Eq a => [a] -> [a] -> Bool
setEq xs ys = (subset xs ys) && (subset ys xs)

union :: Eq a => [a] -> [a] -> [a]
union = foldl (\acc x -> if elem x acc then acc else x : acc)

intersect :: Eq a => [a] -> [a] -> [a]
intersect xs = foldl (\acc y -> if elem y xs then y : acc else acc) []

different :: Eq a => [a] -> [a] -> [a]
different xs ys = foldl (\acc x -> if not $ elem x ys then x : acc else acc) [] xs

palindrome :: [Char] -> Bool
palindrome x = let y = reverse x
               in x == y

sumSquare :: Int -> Int
sumSquare n = sum $ map (^2) [0..n]

pythagoreans :: Int -> [(Int,Int,Int)]
pythagoreans n = [(a,b,c) | c <- [1..n], b <- [1..n], a <- [1..n], a^2 + b^2 == c^2]

uniquePyths :: Int -> [(Int,Int,Int)]
uniquePyths n = [(a,b,c) | c <- [1..n], b <- [1..c], a <- [1..b], a^2 + b^2 == c^2]

perfects :: Int -> [Int]
perfects x = let factors y = [a | a <- [1..(y-1)], (mod y a) == 0]
             in [b | b <- [1..x], (sum $ factors b) == b]

binToInt :: [Char] -> Int
binToInt [] = 0
binToInt xs = let n = (length xs) - 1
                  powers = reverse [2^x | x <- [0..n]]
              in sum $ zipWith (\bin pow -> if bin == '1' then pow else 0) xs powers

intToBin :: Int -> [Char]
intToBin 0 = []
intToBin x
  | mod x 2 == 1 = (intToBin y) ++ ['1']
  | otherwise = (intToBin y) ++ ['0']
  where y = div x 2

signedIntToBin :: Int -> [Char]
signedIntToBin x
  | x < 0 && x >= -2^31 = signedIntToBin (x + 2^32)
  | x >= 0 = intToBin x

toRomanNumeral :: Int -> [Char]
toRomanNumeral x
  | x >= 1000 = 'M' : toRomanNumeral (x - 1000)
  | x < 1000 && x >= 500 = 'D' : toRomanNumeral (x-500)
  | x < 500 && x >= 100 = 'C' : toRomanNumeral (x-100)
  | x < 100 && x >= 50 = 'L' : toRomanNumeral (x-50)
  | x < 50 && x >= 10 = 'X' : toRomanNumeral (x-10)
  | x < 10 && x >= 5 = 'V' : toRomanNumeral (x-5)
  | x < 5 && x >= 1 = 'I' : toRomanNumeral (x-1)
  | otherwise = []

toChange :: Float -> [Char]
toChange x = let target = toChangeHelper x
                 set = listToSet target
                 count y ys = length $ filter (==y) ys
             in foldl1 (++) $ map (\a -> (show $ count a target) ++ "-" ++ a ++ "\n") set

toChangeHelper :: Float -> [String]
toChangeHelper x
  | x >= 100 = "One Hundred" : toChangeHelper (x-100)
  | x < 100 && x >= 20 = "Twenty" : toChangeHelper (x-20)
  | x < 20 && x >= 1 = "One" : toChangeHelper (x-1)
  | x < 1 && x >= 0.25 = "Quarter" : toChangeHelper (x-0.25)
  | x < 0.25 && x >= 0.1 = "Dime" : toChangeHelper (x-0.1)
  | otherwise = []
