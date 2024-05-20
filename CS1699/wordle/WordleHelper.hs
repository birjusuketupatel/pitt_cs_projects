import System.IO

main = do
  --load dictionary of possible words
  handle <- openFile "sgb-words.txt" ReadMode
  content <- hGetContents handle
  let dict = lines content

  --run helper routine
  helper dict "" [] []

--helper dict a b c
--a: characters not in word
--b: characters in word not in given position
--c: characters in word at given position
helper :: [String] -> [Char] -> [(Char,Int)] -> [(Char,Int)] -> IO ()
helper dict a b c = do
  --display conditions we have already filtered on
  putStrLn ("There are " ++ (show $ length dict) ++ " words satisfying the following conditions:")

  let y = foldl (\acc (c,i) -> acc ++ ("  - Contain " ++ show c ++ " but not at position [" ++ show i ++ "]\n")) "" b
  let z = foldl (\acc (c,i) -> acc ++ ("  - Contain " ++ show c ++ " at position [" ++ show i ++ "]\n")) "" c
  putStrLn ("  - Do not contain " ++ a)
  putStr y
  putStr z

  --ask user for input
  putStr "Enter a word and hints (0 to show remaining words): "
  hFlush stdout
  ln <- getLine

  if ln == "0"
  then do
       putStrLn $ show dict
       helper dict a b c
  else do
       let guess = takeWhile (\x -> x /= '-' && x /= '+' && x /= '*') ln
       let hint = dropWhile (\x -> x /= '-' && x /= '+' && x /= '*') ln

       --filter the list given the new hint
       let (newDict, newA, newB, newC) = filterList guess hint 0 dict a b c

       --move to next iteration
       helper newDict newA newB newC

filterList :: String -> String -> Int -> [String] -> String -> [(Char,Int)] -> [(Char,Int)] -> ([String],String,[(Char,Int)],[(Char,Int)])
filterList [] _ _ dict a b c = (dict, a, b, c)
filterList _ [] _ dict a b c = (dict, a, b, c)
filterList (g:guess) (h:hint) pos dict a b c = if h == '-'
                                               then let newDict = filter (\x -> not $ elem g x) dict
                                                    in filterList guess hint (pos + 1) newDict (a ++ [g]) b c
                                               else if h == '+'
                                               then let newDict = filter (\x -> elem g x && (x !! pos) /= g) dict
                                                    in filterList guess hint (pos + 1) newDict a (b ++ [(g,pos)]) c
                                               else if h == '*'
                                               then let newDict = filter (\x -> elem g x && (x !! pos) == g) dict
                                                    in filterList guess hint (pos + 1) newDict a b (c ++ [(g,pos)])
                                               else error "invalid character in hint"
