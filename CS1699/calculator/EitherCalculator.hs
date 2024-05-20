import System.IO
import EitherAlgExp

main = do
  putStrLn "Enter an expression at the prompt (>) or enter 'q' to exit"
  main'

main' = do
  exp <- prompt "> "
  if exp == "q"
    then return ()
    else do (case calculate exp of
              Left x -> putStrLn x
              Right x -> putStrLn ("= " ++ x))
            main'

calculate :: [Char] -> Either [Char] [Char]
calculate str = return str >>= eitherCheckChars >>= eitherTokenizer >>= eitherCheckBalanced >>= eitherInfix2Postfix >>= eitherEvaluate

prompt :: [Char] -> IO [Char]
prompt str = do
    putStr str
    hFlush stdout
    getLine
