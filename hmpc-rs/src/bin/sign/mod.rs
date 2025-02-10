use std::fs::{self, File};
use std::io::Read;
use std::path::Path;

use log::error;
use rcgen::{KeyPair, PKCS_ED25519};

use super::{verbose_info, verbose_warn};


fn create_signing_files(verification_key_filename: &Path, signing_key_filename: &Path, verbose: bool)
{
    let key_pair = KeyPair::generate_for(&PKCS_ED25519).expect("Could not generate keys");

    let verification_key = key_pair.public_key_raw();
    let signing_key = key_pair.serialize_der();

    fs::create_dir_all(
        verification_key_filename
            .parent()
            .expect("Could not determine verification key directory"),
    )
    .expect("Could not create verification key directory");
    fs::create_dir_all(
        signing_key_filename
            .parent()
            .expect("Could not determine singing key directory"),
    )
    .expect("Could not create singing key directory");

    fs::write(verification_key_filename, verification_key).expect("Could not write verification key file");
    fs::write(signing_key_filename, signing_key).expect("Could not write signing key file");

    verbose_info!(verbose, "wrote verification key file: {verification_key_filename:?}");
    verbose_info!(verbose, "wrote signing key file: {signing_key_filename:?}");
}


fn check_signing_files(mut verification_key_file: File, mut signing_key_file: File)
{
    let mut verification_key = Vec::new();
    verification_key_file.read_to_end(&mut verification_key).expect("Could not read verification key");
    let mut signing_key: Vec<_> = Vec::new();
    signing_key_file.read_to_end(&mut signing_key).expect("Could not read signing key");

    error!("Checking that the private key belongs to the verification key is not implemented");
}


pub(crate) fn create_or_check_signing_files(verification_key_filename: &Path, signing_key_filename: &Path, force: bool, verbose: bool)
{
    let verification_key_file = File::open(verification_key_filename);
    let signing_key_file = File::open(signing_key_filename);

    match (verification_key_file, signing_key_file, force)
    {
        (Err(_), Err(_), false) => create_signing_files(verification_key_filename, signing_key_filename, verbose),
        (Ok(verification_key_file), Ok(signing_key_file), false) => check_signing_files(verification_key_file, signing_key_file),
        (Ok(_), Err(_), false) => panic!("Verification key file ({verification_key_filename:?}) exists but signing key file ({signing_key_filename:?}) does not exist. Try to restore the key or create a new pair (--force)."),
        (Err(_), Ok(_), false) => panic!("Verification key file ({verification_key_filename:?}) does not exist but signing key file ({signing_key_filename:?}) exist. Try to restore the key or create a new pair (--force)."),
        (verification_key_file, signing_key_file, true) =>
        {
            verbose_warn!(verbose && verification_key_file.is_ok(), "overwriting verification key file: {verification_key_filename:?}");
            verbose_warn!(verbose && signing_key_file.is_ok(), "overwriting signing key file: {signing_key_filename:?}");
            create_signing_files(verification_key_filename, signing_key_filename, verbose);
        },
    };
}
