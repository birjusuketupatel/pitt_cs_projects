import System.IO
import System.Random

main = do
  --load list of possible words
  handle <- openFile "sgb-words.txt" ReadMode
  content <- hGetContents handle

  --play game in infinite loop
  playGame $ lines content

playGame :: [String] -> IO ()
playGame dict = do
  --display instructions
  putStrLn "Welcome to Wordle by Haskell"
  putStrLn "============================"
  putStrLn "(4) watts"
  putStrLn "    *+---"
  putStrLn "You of 4 more tries."
  putStrLn "'w' is in the word and in the correct spot."
  putStrLn "'a' is in the word but in the wrong spot."
  putStrLn "'t' and 's' are not in the word."

  --pick random word from dictionary
  let len = length dict
  g <- newStdGen
  let i = fst $ randomR (0, len-1) g
  let goal = dict !! i

  --play a round of Wordle
  playRound 6 goal

playRound :: Int -> String -> IO ()
playRound round goal = do
  if round <= 0
  then do
    putStrLn ("You lose...The word is \"" ++ goal ++ "\".")
  else do
    putStr ("(" ++ show round ++ ")" ++ " ")
    hFlush stdout
    guess <- getLine
    let hint = getHint goal guess
    putStrLn ("    " ++ hint)
    if guess == goal
    then do
      putStrLn "You Win..."
    else do
      playRound (round - 1) goal

--getHint "wroth" "motto" -> "-++*+"
getHint :: [Char] -> [Char] -> [Char]
getHint goal guess = foldl (\acc x -> let i = length acc
                                      in if not $ elem x goal
                                         then acc ++ "-"
                                         else if (goal !! i) /= x
                                         then acc ++ "+"
                                         else acc ++ "*"
                           ) "" guess
