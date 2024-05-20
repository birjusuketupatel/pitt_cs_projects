import System.IO
import BasicAlgExp

main = do
  putStrLn "Enter an expression at the prompt (>) or enter 'q' to exit"
  main'

main' = do
  exp <- prompt "> "
  if exp == "q"
    then return ()
    else do
      let result = calculate exp
      if take 5 result == "Error"
        then putStrLn result
        else putStrLn ("= " ++ result)
      main'

calculate :: [Char] -> [Char]
calculate = evaluate . infix2Postfix . checkBalanced . tokenizer . checkChars

prompt :: [Char] -> IO [Char]
prompt str = do
    putStr str
    hFlush stdout
    getLine
