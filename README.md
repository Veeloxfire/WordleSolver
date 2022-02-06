# Wordle Solver
Wordle solver written in c++ (could easily be converted to c probably)

Its unreasonably effective for what it does.
It doesn't do any fancy strategies, it guesses somewhat like a regular human with an irregular vocabulary.
It uses the same "algorithm" the whole time and this could be changed to be different based on how many words left, which letters it knows, etc

From my limited testing it usually guesses correctly in ~4 tries. It can get stuck when there is only 1 letter to guess and multiple options (this is where different strategies would come in)

Currently only supports windows but you could write a linux_main.cpp fairly easily if you wanted

You have to provide your own word list but here is a good one: [twl06](http://norvig.com/ngrams/TWL06.txt)

Copy it yourself ... its a wordle solver
