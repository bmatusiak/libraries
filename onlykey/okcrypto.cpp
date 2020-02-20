/* 
 * Copyright (c) 2015-2020, CryptoTrust LLC.
 * All rights reserved.
 * 
 * Author : Tim Steiner <t@crp.to>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by CryptoTrust LLC. for
 *    the OnlyKey Project (https://crp.to/ok)"
 *
 * 4. The names "OnlyKey" and "CryptoTrust" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    admin@crp.to.
 *
 * 5. Products derived from this software may not be called "OnlyKey"
 *    nor may "OnlyKey" or "CryptoTrust" appear in their names without
 *    specific prior written permission. For written permission, please
 *    contact admin@crp.to.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by CryptoTrust LLC. for
 *    the OnlyKey Project (https://crp.to/ok)"
 *
 * 7. Redistributions in any form must be accompanied by information on
 *    how to obtain complete source code for this software and any
 *    accompanying software that uses this software. The source code
 *    must either be included in the distribution or be available for
 *    no more than the cost of distribution plus a nominal fee, and must
 *    be freely redistributable under reasonable conditions. For a
 *    binary file, complete source code means the source code for all
 *    modules it contains.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS
 * ARE GRANTED BY THIS LICENSE. IF SOFTWARE RECIPIENT INSTITUTES PATENT
 * LITIGATION AGAINST ANY ENTITY (INCLUDING A CROSS-CLAIM OR COUNTERCLAIM
 * IN A LAWSUIT) ALLEGING THAT THIS SOFTWARE (INCLUDING COMBINATIONS OF THE
 * SOFTWARE WITH OTHER SOFTWARE OR HARDWARE) INFRINGES SUCH SOFTWARE
 * RECIPIENT'S PATENT(S), THEN SUCH SOFTWARE RECIPIENT'S RIGHTS GRANTED BY
 * THIS LICENSE SHALL TERMINATE AS OF THE DATE SUCH LITIGATION IS FILED. IF
 * ANY PROVISION OF THIS AGREEMENT IS INVALID OR UNENFORCEABLE UNDER
 * APPLICABLE LAW, IT SHALL NOT AFFECT THE VALIDITY OR ENFORCEABILITY OF THE
 * REMAINDER OF THE TERMS OF THIS AGREEMENT, AND WITHOUT FURTHER ACTION
 * BY THE PARTIES HERETO, SUCH PROVISION SHALL BE REFORMED TO THE MINIMUM
 * EXTENT NECESSARY TO MAKE SUCH PROVISION VALID AND ENFORCEABLE. ALL
 * SOFTWARE RECIPIENT'S RIGHTS UNDER THIS AGREEMENT SHALL TERMINATE IF IT
 * FAILS TO COMPLY WITH ANY OF THE MATERIAL TERMS OR CONDITIONS OF THIS
 * AGREEMENT AND DOES NOT CURE SUCH FAILURE IN A REASONABLE PERIOD OF
 * TIME AFTER BECOMING AWARE OF SUCH NONCOMPLIANCE. THIS SOFTWARE IS
 * PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR  PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,  EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstring>
#include "Arduino.h"
#include "onlykey.h"
#ifdef STD_VERSION
#include <SoftTimer.h>
#include <RNG.h>
#include "sha1.h"
#include "sha512.h"
#include "yubikey.h"
#include "device.h"
#include "okcrypto.h"

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "memory_buffer_alloc.h"
#endif


/*************************************/
//RSA assignments
/*************************************/
uint8_t rsa_publicN[MAX_RSA_KEY_SIZE];
uint8_t rsa_private_key[MAX_RSA_KEY_SIZE];
/*************************************/
//ECC Authentication assignments
/*************************************/
uint8_t ecc_public_key[(MAX_ECC_KEY_SIZE*2)+1];
uint8_t ecc_private_key[MAX_ECC_KEY_SIZE];
/*************************************/
//HMACSHA1 assignments
/*************************************/
extern uint8_t keyboard_buffer[KEYBOARD_BUFFER_SIZE];
/*************************************/


extern uint8_t Challenge_button1;
extern uint8_t Challenge_button2;
extern uint8_t Challenge_button3;
extern uint8_t CRYPTO_AUTH;
uint8_t type;
extern int large_buffer_offset;
extern uint8_t resp_buffer[64];
extern uint8_t* large_buffer;
extern uint8_t recv_buffer[64];
extern int large_buffer_len;
//extern int msgcount;
extern uint8_t profilekey[32];
extern uint8_t packet_buffer_details[5];
extern uint8_t* large_resp_buffer;
extern uint8_t outputmode;
extern uint8_t pending_operation;

void SIGN (uint8_t *buffer) {
	uECC_set_rng(&RNG2);
	uint8_t features = 0;
	#ifdef DEBUG
	Serial.println();
	Serial.println("OKSIGN MESSAGE RECEIVED");
	#endif
	if (buffer[5] < 101) { //Slot 101-132 are for ECC, 1-4 are for RSA
	features = onlykey_flashget_RSA ((int)buffer[5]);
	if (type == 0) {
		hidprint("Error no key set in this slot");
		fadeoff(0);
		return;
	}
	#ifdef DEBUG
	Serial.print(features, BIN);
	#endif
	if (is_bit_set(features, 6)) {
		RSASIGN(buffer);
	} else {
		#ifdef DEBUG
		Serial.print("Error key not set as signature key");
		#endif
		hidprint("Error key not set as signature key");
		fadeoff(0);
		}
		return;
	}
	else if (buffer[5] > 200 && buffer[5] < 204) { //SSH Sign Request
		ECDSA_EDDSA(buffer);
	} else {
	if (buffer[5] != 132 && buffer[5] != 131 && buffer[5] != 130) { //These keys are reserved for derivation, backup, and HMACSHA1
	features = onlykey_flashget_ECC ((int)buffer[5]);
	}
	if (type == 0) {
		hidprint("Error no key set in this slot");
		fadeoff(0);
		return;
	}
	#ifdef DEBUG
	Serial.print(features, BIN);
	#endif
	if (is_bit_set(features, 6)) {
		ECDSA_EDDSA(buffer);
	} else {
		#ifdef DEBUG
		Serial.print("Error key not set as signature key");
		#endif
		hidprint("Error key not set as signature key");
		fadeoff(0);
		return;
	}
	}
}

void GETPUBKEY (uint8_t *buffer) {
	#ifdef DEBUG
	Serial.println();
	Serial.println("OKGETPUBKEY MESSAGE RECEIVED");
	#endif

	if (buffer[5] < 5 && !buffer[6]) { //Slot 101-132 are for ECC, 1-4 are for RSA
		if (onlykey_flashget_RSA ((int)buffer[5])) GETRSAPUBKEY(buffer);
	} else if (buffer[5] < 130 && !buffer[6]) { //132 and 131 and 130 are reserved
		if (onlykey_flashget_ECC ((int)buffer[5])) GETECCPUBKEY(buffer);
	} else if (buffer[6] <= 3) { // Generate key using provided data, return public
	DERIVEKEY(buffer[6], buffer+7, NULL);
	send_transport_response(ecc_public_key, 64, false, false);
	}

}

void DECRYPT (uint8_t *buffer){
	uECC_set_rng(&RNG2);
	uint8_t features = 0;
	#ifdef DEBUG
	Serial.println();
	Serial.println("OKDECRYPT MESSAGE RECEIVED");
	#endif
	if (buffer[5] < 101) { //Slot 101-132 are for ECC, 1-4 are for RSA
	features = onlykey_flashget_RSA (buffer[5]);
	if (type == 0) {
		hidprint("Error no key set in this slot");
		fadeoff(0);
		return;
	}
	if (is_bit_set(features, 5)) {
		RSADECRYPT(buffer);
	} else {
		#ifdef DEBUG
		Serial.print("Error key not set as decryption key");
		#endif
		hidprint("Error key not set as decryption key");
		fadeoff(0);
		return;
	}
	} else {
		if (buffer[5] != 132 && buffer[5] != 131 && buffer[5] != 130) { //These keys are reserved for derivation, backup, and HMACSHA1
		features = onlykey_flashget_ECC ((int)buffer[5]);
		}
    if (type == 0) {
			hidprint("Error no key set in this slot");
		  fadeoff(0);
		return;
	}
	if (is_bit_set(features, 5)) {
		ECDH(buffer);
	} else {
		#ifdef DEBUG
		Serial.print("Error key not set as decryption key");
		#endif
		hidprint("Error key not set as decryption key");
		fadeoff(0);
		return;
	}
	}
}

void GENERATE_KEY (uint8_t *buffer) {
	uECC_set_rng(&RNG2);
	//uint8_t backupslot;
	//uint8_t temp[64];
	#ifdef DEBUG
	Serial.println();
	Serial.println("GENERATE KEY MESSAGE RECEIVED");
	#endif
	if (buffer[5] > 100) { //Slot 101-132 are for ECC, 1-4 are for RSA
		if ((buffer[6] & 0x0F) == 1) {
			RNG2(buffer+7, 32);
		} else if ((buffer[6] & 0x0F) == 2) {
			const struct uECC_Curve_t * curve = uECC_secp256r1(); //P-256
			uECC_make_key(ecc_public_key, buffer+7, curve);
		} else if ((buffer[6] & 0x0F) == 3) {
			const struct uECC_Curve_t * curve = uECC_secp256k1();
			uECC_make_key(ecc_public_key, buffer+7, curve);
		}
		memset(ecc_public_key, 0, sizeof(ecc_public_key));
	}
	return;
}

void GETRSAPUBKEY (uint8_t *buffer)
{
#ifdef DEBUG
	byteprint(rsa_publicN, (type*128));
#endif
	send_transport_response(rsa_publicN, (type*128), false, false);
}

void RSASIGN (uint8_t *buffer)
{
	uint8_t rsa_signature[(type*128)];
	uint8_t rsa_signaturetemp[64];

    if(!CRYPTO_AUTH) process_packets (buffer, 0, 0);
	else if (CRYPTO_AUTH == 4) {
		if (large_buffer_offset != 28 && large_buffer_offset != 32 && large_buffer_offset != 48 && large_buffer_offset != 64) {
		hidprint("Error with RSA data to sign invalid size");
#ifdef DEBUG
    Serial.println("Error with RSA data to sign invalid size");
	Serial.println(large_buffer_offset);
#endif
		fadeoff(1);
		large_buffer_offset = 0;
		memset(large_buffer, 0, LARGE_BUFFER_SIZE); //wipe buffer
		return;
	}
	aes_gcm_decrypt(large_buffer, packet_buffer_details[0], packet_buffer_details[1], profilekey, large_buffer_offset);
#ifdef DEBUG
    Serial.println();
    Serial.printf("RSA data to sign size=%d", large_buffer_offset);
	Serial.println();
	byteprint(large_buffer, large_buffer_offset);
#endif
  pending_operation=CTAP2_ERR_OPERATION_PENDING;
  memcpy(rsa_signaturetemp, large_buffer, large_buffer_offset);
  memset(large_buffer, 0, LARGE_BUFFER_SIZE);
  if (rsa_sign (large_buffer_offset, rsa_signaturetemp, rsa_signature) == 0)
	{
		pending_operation=CTAP2_ERR_DATA_READY;
#ifdef DEBUG
		Serial.print("Signature = ");
	    byteprint(rsa_signature, sizeof(rsa_signature));
#endif
	outputmode=packet_buffer_details[2]; // Outputmode set at start of operation
	send_transport_response(rsa_signature, (type*128), true, true);
	} else {
		pending_operation=0;
		hidprint("Error with RSA signing");
	}
	fadeoff(85);
	memset(rsa_signature, 0, sizeof(rsa_signature));
	if (outputmode != WEBAUTHN) memset(large_resp_buffer, 0, LARGE_RESP_BUFFER_SIZE);
    return;
	} else {
#ifdef DEBUG
    Serial.println("Waiting for challenge buttons to be pressed");
#endif
	pending_operation=CTAP2_ERR_USER_ACTION_PENDING;
	}
}

void RSADECRYPT (uint8_t *buffer)
{

	unsigned int plaintext_len = 0;
    if(!CRYPTO_AUTH) process_packets (buffer, 0, 0);
	else if (CRYPTO_AUTH == 4) {
		if (large_buffer_offset != (type*128)) {
		hidprint("Error with RSA data to decrypt invalid size");
#ifdef DEBUG
    Serial.println("Error with RSA data to decrypt invalid size");
	Serial.println(large_buffer_offset);
#endif
		fadeoff(1);
		large_buffer_offset = 0;
		memset(large_buffer, 0, LARGE_BUFFER_SIZE); //wipe buffer
		return;
	}
	aes_gcm_decrypt(large_buffer, packet_buffer_details[0], packet_buffer_details[1], profilekey, large_buffer_offset);
#ifdef DEBUG
    Serial.println();
    Serial.printf("RSA ciphertext blob size=%d", large_buffer_offset);
	Serial.println();
	byteprint(large_buffer, large_buffer_offset);
#endif
	// decrypt ciphertext in large_buffer to temp_buffer
  pending_operation=CTAP2_ERR_OPERATION_PENDING;
  uint8_t rsa_decrypttemp[(type*128)];
  memcpy(rsa_decrypttemp, large_buffer, large_buffer_offset);
  memset(large_buffer, 0, LARGE_BUFFER_SIZE);
  if (rsa_decrypt (&plaintext_len, rsa_decrypttemp, large_resp_buffer) == 0)
	{
		pending_operation=CTAP2_ERR_DATA_READY;
#ifdef DEBUG
		Serial.println();
		Serial.print("Plaintext len = ");
		Serial.println(plaintext_len);
		Serial.print("Plaintext = ");
		byteprint(large_resp_buffer, plaintext_len);
		Serial.println();
#endif
	outputmode=packet_buffer_details[2]; // Outputmode set at start of operation
	send_transport_response(large_resp_buffer, plaintext_len,  true, true);
	} else {
		pending_operation=0;
		hidprint("Error with RSA decryption");
	}
	fadeoff(85);
	if (outputmode != WEBAUTHN) memset(large_resp_buffer, 0, LARGE_RESP_BUFFER_SIZE);

    return;
	} else {
#ifdef DEBUG
    Serial.println("Waiting for challenge buttons to be pressed");
#endif
	pending_operation=CTAP2_ERR_USER_ACTION_PENDING;
	}
}

void GETECCPUBKEY (uint8_t *buffer)
{
		onlykey_flashget_ECC (buffer[5]);
			#ifdef DEBUG
    	    Serial.println("OKGETECCPUBKEY MESSAGE RECEIVED");
			byteprint(ecc_public_key, sizeof(ecc_public_key));
	    #endif
			send_transport_response(ecc_public_key, 64, false, false);
			memset(ecc_public_key, 0, MAX_ECC_KEY_SIZE*2); //wipe buffer
			memset(ecc_private_key, 0, MAX_ECC_KEY_SIZE); //wipe buffer
}

void GENPUBKEY (uint8_t *buffer)
{
	uint8_t pk[crypto_box_PUBLICKEYBYTES];
	//Generate public key of hash
	//crypto_scalarmult_base(pk, buffer);
	// ^^ Too Slow
	buffer[0] &= 0xF8;
    buffer[31] = (buffer[31] & 0x7F) | 0x40;
	Curve25519::eval(pk, buffer, 0);
	memcpy(buffer, pk, 32);
}

void DERIVEKEY (uint8_t ktype, uint8_t *data, uint8_t slot)
{
  if (!slot) onlykey_flashget_ECC (132); //Default Key stored in ECC slot 32
  else if (slot<125) onlykey_flashget_ECC (slot); 
  memset(ecc_public_key, 0, sizeof(ecc_public_key));
  SHA256_CTX ekey;
  sha256_init(&ekey);
  sha256_update(&ekey, ecc_private_key, 32); //Add default key to ekey
  sha256_update(&ekey, data, 32); //Add provided data to ekey
  sha256_final(&ekey, ecc_private_key); //Create hash and store
	if (ktype==1) {
		Ed25519::derivePublicKey(ecc_public_key, ecc_private_key);
		return;
	}
	else if (ktype==2) {
		const struct uECC_Curve_t * curve = uECC_secp256r1();
		uECC_compute_public_key(ecc_private_key, ecc_public_key, curve);
	}
	else if (ktype==3) {
		const struct uECC_Curve_t * curve = uECC_secp256k1();
		uECC_compute_public_key(ecc_private_key, ecc_public_key, curve);
	}
	/*
	uECC_compress(ecc_public_key, temp, curve);
	memset(ecc_public_key, 0, sizeof(ecc_public_key));
	memcpy(ecc_public_key+31, temp, 33);
	#ifdef DEBUG
	Serial.println("Compressed Public key");
	byteprint(ecc_public_key, sizeof(ecc_public_key));
	#endif
	*/
}

void ECDSA_EDDSA(uint8_t *buffer)
{
	uint8_t ecc_signature[64];
	uint8_t sha256_hash[32];
	uint8_t len = 0;
#ifdef DEBUG
    Serial.println();
    Serial.println("OKECDSA_EDDSACHALLENGE MESSAGE RECEIVED");
#endif
    if(!CRYPTO_AUTH) process_packets (buffer, 0, 0);
	else if (CRYPTO_AUTH == 4) {
	aes_gcm_decrypt(large_buffer, packet_buffer_details[0], packet_buffer_details[1], profilekey, large_buffer_offset);
#ifdef DEBUG
    Serial.println();
    Serial.printf("ECC challenge blob size=%d", large_buffer_offset);
	Serial.println();
    byteprint(large_buffer, large_buffer_offset);
#endif
	uint8_t tmp[32 + 32 + 64];
	SHA256_HashContext ectx = {{&init_SHA256, &update_SHA256, &finish_SHA256, 64, 32, tmp}};
	if (buffer[5] == 201) {
		//Used by SSH, old version used 132, new version uses 201 for type 1
		DERIVEKEY(1, large_buffer+(large_buffer_offset-32), NULL);
		type = 1;
	}
	else if (buffer[5] == 202) {
		DERIVEKEY(2, large_buffer+(large_buffer_offset-32), NULL);
		type = 2;
	}
	else if (buffer[5] == 203) {
		DERIVEKEY(3, large_buffer+(large_buffer_offset-32), NULL);
		type = 3;
	}

	if (large_buffer_offset > 32) large_buffer_offset = large_buffer_offset - 32;

	SHA256_CTX msghash;
	sha256_init(&msghash);
	sha256_update(&msghash, large_buffer, large_buffer_offset);
	sha256_final(&msghash, sha256_hash); //Create hash and store
#ifdef DEBUG
      Serial.println("Signature Hash ");
	  byteprint(sha256_hash, 32);
	  Serial.print("Type");
	  Serial.println(type);
#endif
	if (type==0x01) Ed25519::sign(ecc_signature, ecc_private_key, ecc_public_key, large_buffer, large_buffer_offset);
	else if (type==0x02) {
		const struct uECC_Curve_t * curve = uECC_secp256r1(); //P-256
		if (!uECC_sign_deterministic(ecc_private_key,
						sha256_hash,
						32,
						&ectx.uECC,
						ecc_signature,
						curve)) {
#ifdef DEBUG
      Serial.println("Signature Failed ");
#endif
      }
	}
	else if (type==0x03) {
			const struct uECC_Curve_t * curve = uECC_secp256k1();
		if (!uECC_sign_deterministic(ecc_private_key,
						sha256_hash,
						32,
						&ectx.uECC,
						ecc_signature,
						curve)) {
#ifdef DEBUG
      Serial.println("Signature Failed ");
#endif
      }
	}
/*
	if (type==0x03 || type==0x02) {
	  memset(tmp, 0, sizeof(tmp));
	  tmp[len] = 0x30; //header: compound structure
	  uint8_t *total_len = &tmp[len];
      tmp[len++] = 0x44; //total length (32 + 32 + 2 + 2)
      tmp[len++] = 0x02;  //header: integer

			if (ecc_signature[0]>0x7f) {
			   	tmp[len++] = 33;  //33 byte
				tmp[len++] = 0;
				(*total_len)++; //update total length
			}  else {
				tmp[len++] = 32;  //32 byte
		    }
	  memcpy(tmp+len, ecc_signature, 32); //R value
      len +=32;
      tmp[len++] = 0x02;  //header: integer

			if (ecc_signature[32]>0x7f) {
				tmp[len++] = 33;  //32 byte
				tmp[len++] = 0;
				(*total_len)++;	//update total length
			} else {
				tmp[len++] = 32;  //32 byte
			}
      memcpy(tmp+len, ecc_signature+32, 32); //R value
      len +=32;
	}
	*/
	#ifdef DEBUG
	Serial.print("Signature=");
	byteprint(ecc_signature, 64);
	#endif
	outputmode=packet_buffer_details[2]; // Outputmode set at start of operation
	send_transport_response (ecc_signature, 64, false, false);
  // Stop the fade in
  	fadeoff(85);
    memset(large_buffer, 0, LARGE_BUFFER_SIZE);
	memset(ecc_public_key, 0, sizeof(ecc_public_key)); //wipe buffer
	memset(ecc_private_key, 0, sizeof(ecc_private_key)); //wipe buffer
    return;
	} else {
#ifdef DEBUG
    Serial.println("Waiting for challenge buttons to be pressed");
#endif
	pending_operation=CTAP2_ERR_USER_ACTION_PENDING;
	}
}


void ECDH(uint8_t *buffer)
{
	// This function currently is not used, it generates shared secret and returns no data
    uint8_t ephemeral_pub[MAX_ECC_KEY_SIZE*2];
	uint8_t secret[64] = {0};
	#ifdef DEBUG
    Serial.println();
    Serial.println("OKECDH MESSAGE RECEIVED");
	#endif
    if(!CRYPTO_AUTH) process_packets (buffer, 0, 0);
	else if (CRYPTO_AUTH == 4) {
	memcpy (ephemeral_pub, large_buffer, MAX_ECC_KEY_SIZE*2);
    if (shared_secret(ephemeral_pub, secret)) {
		hidprint("Error with ECC Shared Secret");
		return;
	}
	#ifdef DEBUG
    Serial.println();
    Serial.print("Public key to generate shared secret for");
	byteprint(ephemeral_pub, 64);
    Serial.println();
    Serial.print("ECDH Secret is ");
	for (uint8_t i = 0; i< 32; i++) {
		Serial.print(secret[i],HEX);
	}
	#endif
  //	if (outputU2F) {
	//store_U2F_response(secret, 32);
	//} else{
	//RawHID.send(secret, 0);
	//}
	//delay(100);
    // Reference - https://www.ietf.org/mail-archive/web/openpgp/current/msg00637.html
	// https://fossies.org/linux/misc/gnupg-2.1.17.tar.gz/gnupg-2.1.17/g10/ecdh.c
	// gcry_md_write(h, "\x00\x00\x00\x01", 4);      /* counter = 1 */
    // gcry_md_write(h, secret_x, secret_x_size);    /* x of the point X */
    // gcry_md_write(h, message, message_size);      /* KDF parameters */
	// This is a limitation as we have to be able to fit the entire message to decrypt
	// In this way RSA seems to have an advantage?
	// /* Build kdf_params.  */
    //{
    //IOBUF obuf;
    //
    //obuf = iobuf_temp();
    ///* variable-length field 1, curve name OID */
    //err = gpg_mpi_write_nohdr (obuf, pkey[0]);
    ///* fixed-length field 2 */
    //iobuf_put (obuf, PUBKEY_ALGO_ECDH);
    ///* variable-length field 3, KDF params */
    //err = (err ? err : gpg_mpi_write_nohdr (obuf, pkey[2]));
    ///* fixed-length field 4 */
    //iobuf_write (obuf, "Anonymous Sender    ", 20);
    ///* fixed-length field 5, recipient fp */
    //iobuf_write (obuf, pk_fp, 20);
    //
    //message_size = iobuf_temp_to_buffer (obuf, message, sizeof message);
	/*

	uint8_t hash_alg = large_buffer[0];
	uint8_t *pub_key = large_buffer+1;
	uint8_t *msg = large_buffer+1+32;
	uint8_t counter[] = "\x00\x00\x00\x01";
	uint8_t hash[64];
    mbedtls_sha512_context sha512;
	switch (hash_alg) {
		case 2: //sha256
		SHA256_CTX context;
		sha256_init(&context);
		sha256_update(&context, counter, 4); //add counter
		sha256_update(&context, secret, sizeof(secret)); //add secret
		sha256_update(&context, msg, (large_buffer_offset-1-type)); //add message
		sha256_final(&context, hash);
		break;
		case 3: //sha384
		mbedtls_sha512_init (&sha512);
		mbedtls_sha512_starts (&sha512, 1); //is 384
		mbedtls_sha512_update (&sha512, counter, 4); //add counter
		mbedtls_sha512_update (&sha512, secret, sizeof(secret)); //add secret
		mbedtls_sha512_update (&sha512, msg, (large_buffer_offset-1-type)); //add message
		mbedtls_sha512_finish (&sha512, hash);
		mbedtls_sha512_free (&sha512);
		break;
		case 5: //sha512
		mbedtls_sha512_init (&sha512);
		mbedtls_sha512_starts (&sha512, 0); //is not 384
		mbedtls_sha512_update (&sha512, counter, 4); //add counter
		mbedtls_sha512_update (&sha512, secret, sizeof(secret)); //add secret
		mbedtls_sha512_update (&sha512, msg, (large_buffer_offset-1-sizeof(secret))); //add message
		mbedtls_sha512_finish (&sha512, hash);
		mbedtls_sha512_free (&sha512);
		break;
		default:
		hidprint("Error hash algorithm incorrect");
		return;
	}
	//Send the KEK, client app should know the symmetric encryption alg
	//Depending on the alg the client will drop the uneeded tail of the the key

#ifdef DEBUG
    Serial.println();
    Serial.print("ECDH KEK is ");
	for (int i = 0; i< sizeof(hash); i++) {
		Serial.print(hash[i],HEX);
		}
#endif
    RawHID.send(hash, 0);
	*/

	// TODO finish PGP/GPG compatible ECDH, return decrypted data
	//outputmode=packet_buffer_details[2]; // Outputmode set at start of operation
	//send_transport_response (decrypteddata, len, true, true);

    fadeoff(85);

	memset(large_buffer, 0, LARGE_BUFFER_SIZE);
	memset(secret, 0, sizeof(secret)); //wipe buffer
	memset(ecc_public_key, 0, sizeof(ecc_public_key)); //wipe buffer
	memset(ecc_private_key, 0, sizeof(ecc_private_key)); //wipe buffer
    return;
	} else {
#ifdef DEBUG
    Serial.println("Waiting for challenge buttons to be pressed");
#endif
	}
}

void HMACSHA1 () {
	uint8_t temp[32];
	uint8_t inputlen;
	uint16_t crc;
	extern uint8_t setBuffer[8];
	uint8_t *ptr;
#ifdef DEBUG
	Serial.println();
	Serial.println("GENERATE HMACSHA1 MESSAGE RECEIVED");
#endif
    if (CRYPTO_AUTH == 4) {
			if(!check_crc(keyboard_buffer)) {
			memset(setBuffer, 0, 9);
			memset(keyboard_buffer, 0, KEYBOARD_BUFFER_SIZE);
			return;
		}
		if ((keyboard_buffer[64] & 0x0f) == 0x08 ) { //HMAC Slot 2 selected, 0x08 for slot 2, 0x00 for slot 1
			onlykey_flashget_ECC (129); //ECC slot 129 reserved for HMAC Slot 2 custom key 
		} else {
			onlykey_flashget_ECC (130); 
		}
		if (type == 0) { //Generate a key using the default key in slot 130 if there is no key set
		 onlykey_flashget_ECC (130);
		 // Derive key from SHA256 hash of default key and added data temp
			for(int i=0; i<32; i++) {
				temp[i] = i + (keyboard_buffer[64] & 0x0f);
			}
			DERIVEKEY(0, temp, NULL);
		}
		//Variable buffer size
		if (keyboard_buffer[57] == 0x20 && keyboard_buffer[58] == 0x20 && keyboard_buffer[59] == 0x20 && keyboard_buffer[60] == 0x20 && keyboard_buffer[61] == 0x20 && keyboard_buffer[62] == 0x20 && keyboard_buffer[63] == 0x20) {
			inputlen = 32; //KeepassXC uses 0x20 for empty buffer
		} else if (keyboard_buffer[57] == 0 && keyboard_buffer[58] == 0 && keyboard_buffer[59] == 0 && keyboard_buffer[60] == 0 && keyboard_buffer[61] == 0 && keyboard_buffer[62] == 0 && keyboard_buffer[63] == 0) {
			inputlen = 32; //YubiKey personalization tool uses 0 for empty buffer
		} else {
			inputlen = 64;
		}
#ifdef DEBUG
		Serial.print("HMACSHA1 Input = ");
	    byteprint(keyboard_buffer, 70);
		Serial.print("Input Length");
	    Serial.println(inputlen);
#endif
	//Load HMAC Key
	Sha1.initHmac(ecc_private_key, 20);
	//Generate HMACSHA1
	Sha1.write(keyboard_buffer, inputlen);
	ptr=keyboard_buffer;
	ptr = Sha1.resultHmac();
	memcpy(temp, ptr, 20);
	memset(ecc_private_key, 0, 32);
#ifdef DEBUG
		Serial.print("CRC Input = ");
	    byteprint(temp, 20);
#endif
	//Generate CRC of Output
	crc = yubikey_crc16 (temp, 20);
	memcpy(keyboard_buffer, temp, 7);
	keyboard_buffer[7] = 0xC0; //Part 1 of HMAC
	memcpy(keyboard_buffer+8, temp+7, 7);
	keyboard_buffer[15] = 0xC1; //Part 2 of HMAC
	memcpy(keyboard_buffer+16, temp+14, 6);
	keyboard_buffer[23] = 0xC2; //Part 3 of HMAC
	memset(keyboard_buffer +24, 0, KEYBOARD_BUFFER_SIZE-24);
    // CRC Bytes expected are CRC-16/X-25 but yubikey_crc16 generates CRC-16/MCRF4XX,
    // Weird that firmware uses a different CRC-16 than https://github.com/Yubico/yubikey-personalization/blob/master/ykcore/
	// We can XOR CRC-16/MCRF4XX output to convert to CRC-16/X-25
	crc ^= 0xFFFF;
	keyboard_buffer[22] = crc & 0xFF;
	keyboard_buffer[24] = crc >> 8;
	keyboard_buffer[31] = 0xC3; //Part 4 contains part of CRC and mystery byte keyboard_buffer[28]
	keyboard_buffer[28] = 0x4B;
#ifdef DEBUG
		Serial.print("HMACSHA1 Output = ");
	    byteprint(keyboard_buffer, KEYBOARD_BUFFER_SIZE);
		Serial.print("CRC = ");
		Serial.println(crc);
#endif
    return;
	} else {
#ifdef DEBUG
    Serial.println("Waiting for challenge buttons to be pressed");
#endif
	}
}

int shared_secret (uint8_t *pub, uint8_t *secret) {
	const struct uECC_Curve_t * curve;
	#ifdef DEBUG
	Serial.printf("Shared Secret for type %X ",type);
	#endif
	switch (type) {
	case 1:
		if (crypto_box_beforenm(secret, pub, ecc_private_key)) return 1;
		else return 0;
	case 2:
		curve = uECC_secp256r1();
		if (uECC_shared_secret(pub, ecc_private_key, secret, curve)) return 0;
		else return 1;
	case 3:
		curve = uECC_secp256k1();
		if (uECC_shared_secret(pub, ecc_private_key, secret, curve)) return 0;
		else return 1;
	case 4:
		Curve25519::eval(secret, ecc_private_key, pub);
		return 0;
	default:
		hidprint("Error ECC type incorrect");
		return 1;
	}
}

mbedtls_md_context_t sha512_ctx;

void crypto_sha512_init() {
	mbedtls_md_type_t md_type = MBEDTLS_MD_SHA512;
	mbedtls_md_init (&sha512_ctx);
	mbedtls_md_setup(&sha512_ctx, mbedtls_md_info_from_type(md_type), 0); // 0 = not using HMAC
}

void crypto_sha512_update(const uint8_t * data, size_t len) {
	mbedtls_md_update (&sha512_ctx, data, len);
}

void crypto_sha512_final(uint8_t * hash) {
	mbedtls_md_finish (&sha512_ctx, hash);
	mbedtls_md_free (&sha512_ctx);
}

void aes_crypto_box (uint8_t *buffer, int len, bool open) {
	uint8_t iv[12];
	memset(iv, 0, 12);
	//msgcount++;
	//int ctr = ((msgcount>>24)&0xff) | // move byte 3 to byte 0
	//  ((msgcount<<8)&0xff0000) | // move byte 1 to byte 2
	//  ((msgcount>>8)&0xff00) | // move byte 2 to byte 1
	//  ((msgcount<<24)&0xff000000); // byte 0 to byte 3
	//memcpy(iv, &ctr, 4);
	#ifdef DEBUG
	Serial.print("IV");
	byteprint(iv, 12);
	#endif
	#ifdef DEBUG
	Serial.print("Key");
	byteprint(ecc_private_key, 32);
	#endif
	#ifdef DEBUG
	Serial.print("buffer");
	byteprint(buffer, len);
	#endif
	if (open) {
		aes_gcm_decrypt2 (buffer, iv, ecc_private_key, len);
	}
	else {
		aes_gcm_encrypt2 (buffer, iv, ecc_private_key, len);
	}
}

int rsa_sign (int mlen, const uint8_t *msg, uint8_t *out)
{
	//mbedtls_rsa_self_test(1);
	mbedtls_md_type_t md_type = MBEDTLS_MD_NONE;
	int ret = 0;
	static mbedtls_rsa_context rsa;
    uint8_t rsa_ciphertext[(type*128)];
	mbedtls_mpi P1, Q1, H;
	mbedtls_rsa_init( &rsa, MBEDTLS_RSA_PKCS_V15, 0 );

	mbedtls_mpi_init (&P1);  mbedtls_mpi_init (&Q1);  mbedtls_mpi_init (&H);
	rsa.len = (type*128);
	MBEDTLS_MPI_CHK( mbedtls_mpi_lset (&rsa.E, 0x10001) );

	MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary (&rsa.P, &rsa_private_key[0], ((type*128) / 2) ));
	MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary (&rsa.Q, &rsa_private_key[((type*128) / 2)], ((type*128) / 2) ));
	MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi (&rsa.N, &rsa.P, &rsa.Q) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int (&P1, &rsa.P, 1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int (&Q1, &rsa.Q, 1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi (&H, &P1, &Q1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod (&rsa.D , &rsa.E, &H) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi (&rsa.DP, &rsa.D, &P1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi (&rsa.DQ, &rsa.D, &Q1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod (&rsa.QP, &rsa.Q, &rsa.P) );

	#ifdef DEBUG
	Serial.printf( "\nRSA len = " );
	Serial.println(rsa.len);
	#endif
	ret = mbedtls_rsa_check_privkey( &rsa );
	cleanup:
	mbedtls_mpi_free (&P1);  mbedtls_mpi_free (&Q1);  mbedtls_mpi_free (&H);

   if( ret != 0 )
    {
		#ifdef DEBUG
	  Serial.printf("Error with key check =%d", ret);
	  #endif
		hidprint("Error invalid key, key check failed");
		return -1;
		}
  if (ret == 0)
    {
    #ifdef DEBUG
    Serial.print("RSA sign messege length = ");
	  Serial.println(mlen);
	  #endif
	  if (mlen > ((type*128)-11)) mlen = ((type*128)-11);

		switch (mlen) {
		case 64:
			md_type = MBEDTLS_MD_SHA512;
		break;

		case 48:
			md_type = MBEDTLS_MD_SHA384;
		break;

		case 32:
			md_type = MBEDTLS_MD_SHA256;
		break;

		case 28:
			md_type = MBEDTLS_MD_SHA224;
		break;

		//case 20:
		//	md_type = MBEDTLS_MD_RIPEMD160;
		//break;

		default:
		break;

		}

      ret = mbedtls_rsa_rsassa_pkcs1_v15_sign (&rsa, mbedtls_rand, NULL, MBEDTLS_RSA_PRIVATE, md_type, mlen, msg, rsa_ciphertext);
      #ifdef DEBUG
      Serial.print("Hash Value = ");
	  byteprint((uint8_t *)msg, mlen);
	  #endif
	  memcpy (out, rsa_ciphertext, (type*128));
	  /*int ret2 = mbedtls_rsa_rsassa_pkcs1_v15_verify ( &rsa, NULL, NULL, MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_NONE, mlen, msg, rsa_ciphertext );
	  #ifdef DEBUG
      Serial.print("Hash Value = ");
	  byteprint((uint8_t *)msg, mlen);
	  #endif
	  if( ret2 != 0 ) {
		  #ifdef DEBUG
		  Serial.print("Signature Verification Failed ");
		  Serial.println(ret2);
		  #endif
		  return -1;
	  }*/
    }
  mbedtls_rsa_free (&rsa);
  if (ret == 0)
    {
    #ifdef DEBUG
    Serial.println("completed successfully");
	#endif
    return 0;
    }
  else
    {
		#ifdef DEBUG
		Serial.print("MBEDTLS_ERR_RSA_XXX error code ");
    Serial.println(ret);
		#endif
		hidprint("invalid data, or data does not match key");
    return -1;
    }
}

int rsa_decrypt (unsigned int *olen, const uint8_t *in, uint8_t *out)
{
  mbedtls_mpi P1, Q1, H;
  int ret = 0;
  static mbedtls_rsa_context rsa;
  #ifdef DEBUG
  Serial.printf ("\nRSA decrypt:");
  Serial.println ((uint32_t)&ret);
  #endif

  mbedtls_rsa_init (&rsa, MBEDTLS_RSA_PKCS_V15, 0);
  mbedtls_mpi_init (&P1);  mbedtls_mpi_init (&Q1);  mbedtls_mpi_init (&H);
  rsa.len = (type*128);
  MBEDTLS_MPI_CHK( mbedtls_mpi_lset (&rsa.E, 0x10001) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary (&rsa.P, &rsa_private_key[0], ((type*128) / 2) ));
  MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary (&rsa.Q, &rsa_private_key[((type*128) / 2)], ((type*128) / 2) ));
  MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi (&rsa.N, &rsa.P, &rsa.Q) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int (&P1, &rsa.P, 1) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int (&Q1, &rsa.Q, 1) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi (&H, &P1, &Q1) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod (&rsa.D , &rsa.E, &H) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi (&rsa.DP, &rsa.D, &P1) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi (&rsa.DQ, &rsa.D, &Q1) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod (&rsa.QP, &rsa.Q, &rsa.P) );
  #ifdef DEBUG
  Serial.println (rsa.len);
  #endif
  cleanup:
  mbedtls_mpi_free (&P1);  mbedtls_mpi_free (&Q1);  mbedtls_mpi_free (&H);

	#ifdef DEBUG
	Serial.printf( "\nRSA len = " );
	Serial.println(rsa.len);
	#endif
  ret = mbedtls_rsa_check_privkey( &rsa );
	if (ret != 0) {
	  #ifdef DEBUG
      Serial.print ("MBEDTLS_ERR_RSA_XXX error code ");
	  Serial.println (ret);
	  #endif
		hidprint("Error invalid key, key check failed");
	}
  if (ret == 0)
    {
	  #ifdef DEBUG
      Serial.print ("RSA decrypt ");
	  #endif
      ret = mbedtls_rsa_rsaes_pkcs1_v15_decrypt (&rsa, NULL, NULL, MBEDTLS_RSA_PRIVATE, olen, in, out, 512);
    }
  mbedtls_rsa_free (&rsa);
  if (ret == 0)
    {
	  #ifdef DEBUG
      Serial.println ("completed successfully");
	  Serial.print (*olen);
	  #endif
      return 0;
    }
  else
    {
  	#ifdef DEBUG
    Serial.print ("MBEDTLS_ERR_RSA_XXX error code ");
  	Serial.println (ret);
  	#endif
		hidprint("Error invalid data, or data does not match key");
    return -1;
    }
}


void rsa_getpub (uint8_t type)
{
  mbedtls_mpi P, Q, N;
  int ret = 0;
  #ifdef DEBUG
  Serial.print ("RSA generate public N:");
  #endif

  mbedtls_mpi_init (&P);  mbedtls_mpi_init (&Q);  mbedtls_mpi_init (&N);

  MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary (&P, &rsa_private_key[0], ((type*128) / 2) ));
  MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary (&Q, &rsa_private_key[((type*128) / 2)], ((type*128) / 2) ));
  MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi (&N, &P, &Q) );
  MBEDTLS_MPI_CHK( mbedtls_mpi_write_binary (&N, &rsa_publicN[0], (type*128) ));
  cleanup:
  mbedtls_mpi_free (&P);  mbedtls_mpi_free (&Q);  mbedtls_mpi_free (&N);

  if (ret == 0)
    {
	  #ifdef DEBUG
      Serial.print ("Storing RSA public ");
	  byteprint(rsa_publicN, (type*128));
	  #endif
    } else {
		  		hidprint("Error generating RSA public N");
	  #ifdef DEBUG
      Serial.print ("Error generating RSA public");
	  byteprint(rsa_publicN, (type*128));
	  #endif
	}
}

int rsa_encrypt (int len, const uint8_t *in, uint8_t *out)
{
	mbedtls_mpi P1, Q1, H;
	int ret = 0;
	static mbedtls_rsa_context rsa;
	#ifdef DEBUG
	Serial.printf ("\nRSA encrypt:");
	Serial.println ((uint32_t)&ret);
	#endif

	mbedtls_rsa_init (&rsa, MBEDTLS_RSA_PKCS_V15, 0);
	mbedtls_mpi_init (&P1);  mbedtls_mpi_init (&Q1);  mbedtls_mpi_init (&H);
	rsa.len = (type*128);
	MBEDTLS_MPI_CHK( mbedtls_mpi_lset (&rsa.E, 0x10001) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary (&rsa.P, &rsa_private_key[0], ((type*128) / 2) ));
	MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary (&rsa.Q, &rsa_private_key[((type*128) / 2)], ((type*128) / 2) ));
	MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi (&rsa.N, &rsa.P, &rsa.Q) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int (&P1, &rsa.P, 1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int (&Q1, &rsa.Q, 1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi (&H, &P1, &Q1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod (&rsa.D , &rsa.E, &H) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi (&rsa.DP, &rsa.D, &P1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi (&rsa.DQ, &rsa.D, &Q1) );
	MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod (&rsa.QP, &rsa.Q, &rsa.P) );
	#ifdef DEBUG
	Serial.println (rsa.len);
	#endif
	cleanup:
	mbedtls_mpi_free (&P1);  mbedtls_mpi_free (&Q1);  mbedtls_mpi_free (&H);

	#ifdef DEBUG
	Serial.printf( "\nRSA len = " );
	Serial.println(rsa.len);
	#endif
  ret = mbedtls_rsa_check_pubkey( &rsa );
	if (ret != 0) {
	  #ifdef DEBUG
      Serial.print ("MBEDTLS_ERR_RSA_XXX error code ");
	  Serial.println (ret);
	  #endif
	  return ret;
	}
  if (ret == 0)
    {

	  ret = mbedtls_rsa_rsaes_pkcs1_v15_encrypt	( &rsa, mbedtls_rand, NULL, MBEDTLS_RSA_PUBLIC, len,
                           in, out );
    }
  mbedtls_rsa_free (&rsa);
  if (ret == 0)
    {
	  #ifdef DEBUG
      Serial.print ("completed successfully");
	  #endif
      return 0;
    }
  else
    {
	  #ifdef DEBUG
      Serial.print ("MBEDTLS_ERR_RSA_XXX error code ");
	  Serial.println (ret);
	  #endif
      return ret;
    }
}


bool is_bit_set(unsigned char byte, int index) {
  return (byte >> index) & 1;
}

int mbedtls_rand( void *rng_state, unsigned char *output, size_t len )
{
	if( rng_state != NULL )
        rng_state = NULL;
    RNG2( output, len );
    return( 0 );
}

/*
void newhope_test ()
{
	//unsigned long ran;
	char rand[32];
	csprng SRNG,CRNG;
	RAND_clean(&SRNG);
	RAND_clean(&CRNG);
	char s[1792],sb[1824],uc[2176],keyA[32],keyB[32];

	octet S= {0,sizeof(s),s};
	octet SB= {0,sizeof(sb),sb};
	octet UC= {0,sizeof(uc),uc};
	octet KEYA={0,sizeof(keyA),keyA};
	octet KEYB={0,sizeof(keyB),keyB};
	RNG2((uint8_t*)rand, 32);
	RAND_seed(&SRNG, rand);
	RNG2((uint8_t*)rand, 32);
	RAND_seed(&CRNG, rand);

	// NewHope Simple key exchange

	NHS_SERVER_1(&SRNG,&SB,&S);
	NHS_CLIENT(&CRNG,&SB,&UC,&KEYB);
	NHS_SERVER_2(&S,&UC,&KEYA);
#ifdef DEBUG
	Serial.println("NewHope Simple Implemetation from open source AMCL crypto library (https://github.com/MIRACL/amcl)");
	Serial.println("Ref. Alkim, Ducas, Popplemann and Schwabe (https://eprint.iacr.org/2016/1157)");
	Serial.printf("Alice shared secret= 0x");
	byteprint((uint8_t*)KEYA.val, KEYA.len);
	Serial.printf("Bob's shared secret= 0x");
	byteprint((uint8_t*)KEYB.val, KEYB.len);
#endif
	return;
} */

#endif
