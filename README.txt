RENA CANGA 		1115201700218

DiseaseAggregator
Comunication protocol:
	Οι workers στέλνουν πρώτα το port στο οποίο περιμένουν του server για Query Solution. Μετά στέλνουν statistics και όταν τελειώσει στέλνει το string done για να πει στο server 
	ότι τελείωσε η μετάδοση των statistics από τον συγκεκριμένο worker. Ο server αφού έχει πάρει την απάντηση από τον worker την στέλνει στον client μαζί με ένα string done για να
	πει στο client ότι έχει στήλη την απάντηση. Όταν ο client πάρει το sign(done) αλλάζει την μεταβλητή gotAnswer σε 1 και σπάει η loop while. 

Signals:
	Οι server worker και Master κάνουν handle μόνο το SIGINT. Αν πάρουν SIGINT αλλάζουν την τιμή της global μεταβλητής shutdown σε 1, ή οποία κάνει break την ατέρμων loop των server 
	και worker και απελευθερώνουν την μνήμη.

Procedure:
	Πρώτος ξεκινάει ο server ο οποίος κάνει blind τα δυο sockets (statistic, Query) και με μια select περιμένει(accept) νέες συνδέσεις. Αν τα request έρχονται από το 	statisticsSocket τότε είναι οι workers που ζητάνε να συνδεθούν για να δώσουν Statistics και το port που ακούνε, αν έρχονται από το QueryStatistics είναι whoClients. Κάθε accept το 	τοποθετούμε στο global πίνακα τύπου SocketsFds fds, το οποίο είναι ένα array από structs με ένα fd και τον τύπο του fd(WORKER ή CLIENT) ώστε να ξέρουν τα threads πως να τα 		διαχειριστούν. 	Δεύτερος ξεκινάει ο master και οι workers οι οποίοι αφότου στείλουν stats περιμένουν τον Server να συνδεθεί για την εξυπηρέτηση των Queries. Τελευταίος ο 		whoClient ο οποίος όταν φτιάχνει τα threads αυτά δεν ξεκινάνε αμέσως αλλά περιμένουν να δημιουργηθούν όλα τα threads.

Query : 
	listCountries -> Διαβάζει τον πίνακα που έχει αποθηκεύσει τα Directories το συνθέτει σε ένα string με το τερματικό σύμβολο και το στέλνει στον parent.
	diseaseFrequency -> Αν δοθεί Country ψάχνει στο Hashtable by Country, βρίσκει την χώρα και μετράει τους ασθενείς. Αν το Country = NULL ψάχνει στο Hashtable by Disease.
	topkAgeRanges -> Ψάχνει στο Hashtable by Country βρίσκει την χώρα και μετράει τους ασθενείς από το RBT 
	searchPatientRecord -> Διατρέχει την λίστα, αν βρεί το id το γράφει στο pipe, αλλιώς γράφει Error.
	numPatientDischarges -> Αν δοθεί Country ψάχνει στο Hashtable by Country, βρίσκει την χώρα και μετράει τους ασθενείς που έχουν exitdate. Αν το Country NULL διαβάζει όλο το 		Hashtable by Country και μετράει τους ασθενείς που έχουν 	exitdate για όλες τις χώρες.
	numPatientAdmissions -> Same as above... αλλά μετράει τα ENTRY με την ασθένεια disease.
	exit -> Απελευθερώνει τη μνήμη και στέλνει στον parent ένα μήνυμα ότι τελέιωσε, περιμένει ο parent να του στείλει SIGKILL
