module EitherAlgExp
  (eitherCheckChars,
   eitherTokenizer,
   eitherCheckBalanced,
   eitherInfix2Postfix,
   eitherEvaluate) where

eitherCheckChars :: [Char] -> Either [Char] [Char]
eitherCheckChars str = let validChar c = elem c "0123456789+-*/()[]{} "
                           invalidChars = filter (not . validChar) str
                           isValid = (length invalidChars) == 0
                           errorMsg = "Error [checkChars]: The character '" ++ (take 1 invalidChars) ++ "' is not allowed."
                       in if isValid
                          then Right str
                          else Left errorMsg

eitherTokenizer :: [Char] -> Either [Char] [[Char]]
eitherTokenizer x = let isError = (take 5 x) == "Error"
                        y = foldl (\(ls, num) c -> if elem c " +-*/()[]{}"
                                                   then if num /= ""
                                                        then ([c]:num:ls, "")
                                                        else ([c]:ls, "")
                                                   else (ls, num ++ [c]))
                                  ([], "") x
              in if isError
                 then Left x
                 else if (length $ snd y) /= 0
                      then Right (filter (/= " ") $ reverse $ (snd y):(fst y))
                      else Right (filter (/= " ") $ reverse $ fst y)

eitherCheckBalanced :: [[Char]] -> Either [Char] [[Char]]
eitherCheckBalanced [] = Right []
eitherCheckBalanced x = let isError = (take 5 $ head x) == "Error"
                            ls = foldl (\stack c -> if elem c ["(", "[", "{"]
                                                    then c:stack
                                                    else if elem c [")", "]", "}"]
                                                         then if (length stack) == 0
                                                              then "a":stack
                                                              else if (c == ")" && (head stack) == "(") ||
                                                                      (c == "]" && (head stack) == "[") ||
                                                                      (c == "}" && (head stack) == "{")
                                                                   then (tail stack)
                                                                   else "a":stack
                                                   else stack) [] x
                            isBalanced = (length ls) == 0
                            errorMsg = "Error [checkBalanced]: The expression is unbalanced"
                        in if isError
                           then Left (head x)
                           else if isBalanced
                                then Right x
                                else Left errorMsg

--returns if a string expresses an integer
isNum :: [Char] -> Bool
isNum = all (\c -> elem c "0123456789")

--given a stack of operators and an operator
--pops operators from stack with a lower
--returns a list of operators and a new stack
popOperators :: [[Char]] -> [Char] -> ([[Char]], [[Char]])
popOperators [] op = ([], [op])
popOperators (s:stack) op = if (precedenceOrder s) >= (precedenceOrder op)
                            then let (ls, newStack) = popOperators stack op
                                 in  (s:ls, newStack)
                            else ([], op:s:stack)

--given a stack, pops operators until an open parentheses is found
--returns a lost of operators popped and a new stack
popParentheses :: [[Char]] -> ([[Char]], [[Char]])
popParentheses [] = error "The expression is not balanced."
popParentheses (s:stack) = if s == "("
                           then ([], stack)
                           else let (ls, newStack) = popParentheses stack
                                in (s:ls, newStack)

--precedence order: /, *, +, -, (
--given an operator, gets its precedence number
precedenceOrder :: [Char] -> Int
precedenceOrder [a] = if elem a "*/"
                      then 2
                      else if elem a "+-"
                      then 1
                      else 0

eitherInfix2Postfix :: [[Char]] -> Either [Char] [[Char]]
eitherInfix2Postfix x = let isError = (take 5 $ head x) == "Error"
                            (expression, stack) = foldl (\(e, s) i -> if isNum i
                                                                      then (e ++ [i], s)
                                                                      else if elem (head i) "+-*/"
                                                                           then let (ls, newStack) = popOperators s i
                                                                                in (e ++ ls, newStack)
                                                                           else if elem (head i) "([{"
                                                                                then (e, "(":s)
                                                                                else if elem (head i) ")]}"
                                                                                     then let (ls, newStack) = popParentheses s
                                                                                          in (e ++ ls, newStack)
                                                                                     else error "Invalid symbol.")
                                                        ([], []) x
                  in if isError
                     then Left (head x)
                     else Right (expression ++ stack)

eitherEvaluate :: [[Char]] -> Either [Char] [Char]
eitherEvaluate x = let isError = (take 5 $ head x) == "Error"
                       result = foldl (\stack x -> if isNum x
                                                   then x:stack
                                                   else if (length stack) >= 2
                                                        then let [a, b] = map read $ take 2 stack :: [Int]
                                                                 newStack = drop 2 stack
                                                             in if x == "+"
                                                                then (show (b + a)):newStack
                                                                else if x == "-"
                                                                then (show (b - a)):newStack
                                                                else if x == "*"
                                                                then (show (b * a)):newStack
                                                                else (show (div b a)):newStack
                                                                else stack ++ ["a"])
                                [] x
                   in if isError
                      then Left (head x)
                      else if (length result) == 1
                           then Right (head result)
                           else Left "Error [evaluate]: Too many operator(s)"

{-
--This is how the function eitherCalculate is defined in BasicCalculator.hs.

eitherCalculate :: [Char] -> Either [Char] [Char]
eitherCalculate str = pure str >>=
                      eitherCheckChars >>=
                      eitherTokenizer >>=
                      eitherCheckBalanced >>=
                      eitherInfix2Postfix >>=
                      eitherEvaluate
-}
