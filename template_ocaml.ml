type minion = {owner:int ; x:int ; y:int ; load:int ; hp:int ; capacity:int ; atk:int} ;;

let ptr = open_in "mapData.txt" in

let mapSize = input_line ptr |> int_of_string in
let (map : (string * int * int) array array) = Array.init mapSize (fun i ->
  input_line ptr
    |> String.split_on_char ' '
    |> List.map (String.split_on_char ',')
    |> List.map (function | (t::r::m::[]) -> (t,int_of_string r,int_of_string m) | _ -> failwith "invalid 1")
    |> Array.of_list
) in
let nMinions = input_line ptr |> int_of_string in
let minions = Array.init nMinions (fun i ->
  input_line ptr
    |> String.split_on_char ','
    |> List.map int_of_string
    |> (function | (o::x::y::l::h::c::a::[]) -> {owner=o ; x=x ; y=y ; load=l ; hp=h ; capacity=c ; atk=a} | _ -> failwith "invalid 2")
) in
let myID, myResources = input_line ptr |> String.split_on_char ' ' |> List.map int_of_string |> (function | (a::b::[]) -> a,b | _ -> failwith "invalid 3") in
let baseX, baseY = input_line ptr |> String.split_on_char ' ' |> List.map int_of_string |> (function | (a::b::[]) -> a,b | _ -> failwith "invalid 4") in
let curTurn,maxTurn = input_line ptr |> String.split_on_char ' ' |> List.map int_of_string |> (function | (a::b::[]) -> a,b | _ -> failwith "invalid 5") in
let randomEvents = input_line ptr
  |> String.split_on_char ' '
  |> List.map (String.split_on_char ',')
  |> List.map (function |a::b::[] -> (int_of_string a, int_of_string b) | _ -> (-1, -1))
  |> List.filter (fun (a,b) -> a <> -1 || b <> -1)
  |> Array.of_list 
in

close_in ptr;

(*
TYPE_MUR = 'W'
TYPE_NORMAL = 'R'
TYPE_DASH = 'D'
TYPE_SHIELD = 'S'
TYPE_FORCE = 'F'
TYPE_MIDAS = 'M'
TYPE_VITESSE = 'P'
*)

let ptrOut = open_out "answer.txt" in

Array.iter (fun minion ->
  if minion.owner = myID then begin
    (* do things here *)
  end
) minions;

close_out ptrOut