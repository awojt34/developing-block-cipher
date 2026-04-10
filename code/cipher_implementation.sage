from sage.crypto.sbox import SBox
import numpy as np

R_poly.<x> = PolynomialRing(GF(2))
wielomian = x^4 + x^3 + 1
F = GF(2^4, name='x', modulus=wielomian, repr='int')

element_0 = 12
SBOX_ORDER = [3, 2, 1, 0, 7, 5, 6, 4] * 5
ELEMENT_0 = [12, 15, 3, 4, 11, 8, 7, 9]

userKey_hex = "770A8A65DA156D24EE2A093277530142"


#Funkcje pomocnicze 
def int_to_sbox_list(block_int):  
    result = []
    for i in range(31, -1, -1): 
        val = (block_int >> (i * 4)) & 0xF
        result.append(val)
    return result


def sbox_list_to_int(sbox_list):
    result = 0
    for i, val in enumerate(sbox_list):
        result |= (int(val) << ((31 - i) * 4))
    return result


def int_to_gf2_vector(n, bits=32):
    return vector(GF(2), [(n >> i) & 1 for i in range(bits)])


def gf2_vector_to_int(v):
    result = 0
    for i, bit in enumerate(v):
        result |= (int(bit) << i)
    return result


#Generowanie kluczy rundowych
def gen_prekeys(userKey_int):

    keyWords = [
    (userKey_int >> 96) & 0xFFFFFFFF,
    (userKey_int >> 64) & 0xFFFFFFFF,
    (userKey_int >> 32) & 0xFFFFFFFF,
    userKey_int & 0xFFFFFFFF]

    phi = int((sqrt(5) + 1) / 2 * (2**32)) & 0xFFFFFFFF
    preKeys = list(keyWords)
    
    for i in range(4, 132):
        w_i4 = preKeys[i-4]
        w_i3 = preKeys[i-3]
        w_i2 = preKeys[i-2]
        w_i1 = preKeys[i-1]
        
        v_i4 = int_to_gf2_vector(w_i4)
        v_i3 = int_to_gf2_vector(w_i3)
        v_i2 = int_to_gf2_vector(w_i2)
        v_i1 = int_to_gf2_vector(w_i1)
        v_phi = int_to_gf2_vector(phi)
        v_i = int_to_gf2_vector(i)
        
        w_xor_vector = v_i4 + v_i3 + v_i2 + v_i1 + v_phi + v_i
        
        w_xor = gf2_vector_to_int(w_xor_vector)
        wi = ((w_xor << 11) | (w_xor >> 21)) & 0xFFFFFFFF
        preKeys.append(wi)
    
    return preKeys


def gen_sboxes(ELEMENT_0):
    R.<x> = PolynomialRing(GF(2))
    wielomian = x^4 + x^3 + 1
    F = GF(2**4, name="x", modulus=wielomian)
    tab = []

    sboxes = []

    for element_0 in ELEMENT_0:
        tab = [element_0] 
    
        for i in range(1, 16):
            element_F = F.from_integer(i)
            element_inv = element_F ** (-1)
            element_int = element_inv._integer_representation()
            element_xor = element_int ^^ element_0
            tab.append(element_xor)
    
        sboxes.append(SBox(tab))
    return sboxes 


def sbox_operation(sbox, w_i):
    result = 0
    for j in range(0, 32, 4):
        oneSbox = (w_i >> j) & 0xF
        sbox_output = sbox[oneSbox]
        result |= (sbox_output << j)
    return result & 0xFFFFFFFF


def transform_prekeys(preKeys, sboxes):
    subKeys = []
    sbox_usage = [] 
    
    for i in range(33):
        sbox_index = SBOX_ORDER[i % 8]
        sbox = sboxes[sbox_index]
        start = i * 4
        sbox_input = preKeys[start:start+4]       
        subKey = [sbox_operation(sbox, w_i) for w_i in sbox_input]
        subKeys.extend(subKey)
        sbox_usage.append((i, sbox_index, ELEMENT_0
    [sbox_index]))
    
    return subKeys


def gen_roundKeys(userKey_hex):
    userKey_int = int(userKey_hex, 16) 
    preKeys = gen_prekeys(userKey_int)
    sboxes = gen_sboxes(ELEMENT_0)
    subKeys = transform_prekeys(preKeys, sboxes)   
    roundKeys = []
    for i in range(0, len(subKeys), 4):
        k0, k1, k2, k3 = subKeys[i], subKeys[i+1], subKeys[i+2], subKeys[i+3]
        round_key = (k0 << 96) | (k1 << 64) | (k2 << 32) | k3
        roundKeys.append(round_key)
    
    return roundKeys


#Algorytm - budowa

#Konfuzja
def gen_sbox(e0, poly):
    tab = [e0]
    for i in range(1, 16):
        element_F = F.from_integer(i)
        element_inv = element_F ** (-1)
        element_int = element_inv._integer_representation()
        element_xor = element_int ^^ e0
        tab.append(element_xor)
    return SBox(tab)

def sbox_substitution(data_list, sbox):
    return [sbox(val) for val in data_list]


#Dyfuzaja 
def gen_mds():
    R_w.<w> = PolynomialRing(F)
    a = F.gen()
    g = 1
    for i in range(1, 5):
        g = g * (w + a^i)
    
    tab_rows = []
    for i in range(4, 8):
        reszta = (w^i) % g
        coeffs = reszta.list()
        coeffs.reverse()
        tab_rows.append(list(coeffs))
    tab_rows = tab_rows[::-1]
    A = Matrix(F, tab_rows)
    return A


def shift(data_list, n_shift_bits):
    n = len(data_list)
    res = [0] * n
    for i in range(n):
        res[i] = data_list[(i + n_shift_bits) % n]
    return res

def dyfuzja(data_list, matrix):
    result = []
    for i in range(0, len(data_list), 4):
        block = data_list[i:i+4]
        v = vector(F, [F.from_integer(int(x)) for x in block])
        transformed_v = matrix * v
        result.extend([int(x._integer_representation()) for x in transformed_v])
    return result


def algorytm_param(userKey_hex):
    round_keys = gen_roundKeys(userKey_hex)
    S = gen_sbox(element_0, wielomian)
    mds_matrix = gen_mds()
    S_inv = S.inverse()
    mds_matrix_inv = mds_matrix.inverse()
    return round_keys, S, mds_matrix, S_inv, mds_matrix_inv


def encrypt_block(plaintext, round_keys, S, mds_matrix, rounds_num): 
    plaintext = int(plaintext, 16)
    for i in range(rounds_num):
        added_key = plaintext ^^ round_keys[i]
        shifted = shift(int_to_sbox_list(added_key), 5)
        substitution = sbox_substitution(shifted, S)
        diffusion = dyfuzja(substitution, mds_matrix)
        plaintext = sbox_list_to_int(diffusion)
    return plaintext


def decrypt_block(ciphertext, round_keys, S_inv, mds_matrix_inv, rounds_num): 
    for i in range(rounds_num-1, -1, -1):
        podzial = int_to_sbox_list(ciphertext)
        inverse_diffusion = dyfuzja(podzial, mds_matrix_inv)
        inverse_substitution = sbox_substitution(inverse_diffusion, S_inv)
        shifted_back = shift(inverse_substitution, -5)
        added_key = sbox_list_to_int(shifted_back) ^^ round_keys[i]
        ciphertext = added_key
    return ciphertext


def encrypt_message(plaintext_hex, round_keys, S, mds_matrix, rounds_num=33):
    if len(plaintext_hex) % 32 != 0:
        padding_length = 32 - (len(plaintext_hex) % 32)
        plaintext_hex += '0' * padding_length  
    ciphertext_hex = ""
    for i in range(0, len(plaintext_hex), 32):
        block_hex = plaintext_hex[i:i+32]
        block_cipher_int = encrypt_block(block_hex, round_keys, S, mds_matrix, rounds_num)
        ciphertext_hex += f"{block_cipher_int:032X}"
    
    return ciphertext_hex


def decrypt_message(ciphertext_hex, round_keys, S_inv, mds_matrix_inv, rounds_num=33):
    plaintext_hex = ""
    for i in range(0, len(ciphertext_hex), 32):
        block_hex = ciphertext_hex[i:i+32]
        block_plain = decrypt_block(int(block_hex, 16), round_keys, S_inv, mds_matrix_inv, rounds_num)
        plaintext_hex += f"{block_plain:032X}"  
    return plaintext_hex


def test_ad2():
    round_keys, S, mds_matrix, S_inv, mds_matrix_inv = algorytm_param(userKey_hex)
    plaintext = "0123456789ABCDEF0123456789ABCDEF"
    print("Tekst jawny:",plaintext)
    cipher = encrypt_block(plaintext, round_keys, S, mds_matrix, rounds_num=33)
    print(f"Szyfrogram - blok: {cipher:032X}")
    decrypted = decrypt_block(cipher, round_keys, S_inv, mds_matrix_inv, rounds_num=33)
    print(f"Odszyfrowano - blok: {decrypted:032X}")
    message_hex = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"
    print("Tekst jawny - wiadomość:",message_hex)
    ciphertext = encrypt_message(message_hex, round_keys, S, mds_matrix, rounds_num=33)
    print(f"Szyfrogram - wiadomosc: {ciphertext}")   
    decrypted_message = decrypt_message(ciphertext, round_keys, S_inv, mds_matrix_inv, rounds_num=33)
    print(f"Odszyfrowano - wiadomosc: {decrypted_message}")

        
if __name__ == "__main__":
    test_ad2()

