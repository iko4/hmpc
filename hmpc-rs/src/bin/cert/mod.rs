use std::fs::{self, File};
use std::io::Read;
use std::path::Path;

use hmpc_rs::net::Config;
use log::error;
use rcgen::{CertificateParams, CertifiedKey, KeyPair, PKCS_ED25519};
use rustls::pki_types::pem::PemObject;
use rustls::pki_types::{CertificateDer, PrivateKeyDer};

use super::{verbose_info, verbose_warn, Cert, Common};

fn create_certificate_files(certificate_filename: &Path, key_filename: &Path, name: &str, verbose: bool) -> CertifiedKey
{
    let key_pair = KeyPair::generate_for(&PKCS_ED25519).expect("Could not generate keys");
    let params = CertificateParams::new(vec![name.into()]).expect("Could not generate certificate parameters");
    let certificate = params.self_signed(&key_pair).expect("Could not generate self signed certificate");

    let cert = certificate.der();
    let key = key_pair.serialized_der();

    fs::create_dir_all(
        certificate_filename
            .parent()
            .expect("Could not determine certificate directory"),
    )
    .expect("Could not create certificate directory");
    fs::create_dir_all(
        key_filename
            .parent().
            expect("Could not determine key directory")
    )
    .expect("Could not create key directory");

    fs::write(certificate_filename, cert).expect("Could not write certificate file");
    fs::write(key_filename, key).expect("Could not write key file");

    verbose_info!(verbose, "wrote certificate file: {certificate_filename:?}");
    verbose_info!(verbose, "wrote key file: {key_filename:?}");

    CertifiedKey { cert: certificate, key_pair }
}

fn create_signing_files(cert: CertifiedKey, verification_key_filename: &Path, signing_key_filename: &Path, verbose: bool)
{
    let key_pair = cert.key_pair;
    assert_eq!(key_pair.algorithm(), &PKCS_ED25519);

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
            .parent().
            expect("Could not determine singing key directory")
    )
    .expect("Could not create singing key directory");

    fs::write(verification_key_filename, verification_key).expect("Could not write verification key file");
    fs::write(signing_key_filename, signing_key).expect("Could not write signing key file");

    verbose_info!(verbose, "wrote verification key file: {verification_key_filename:?}");
    verbose_info!(verbose, "wrote signing key file: {signing_key_filename:?}");
}

fn check_certificate_files(mut certificate_file: File, mut key_file: File, _name: &str)
{
    let mut cert = Vec::new();
    certificate_file.read_to_end(&mut cert).expect("Could not read certificate");
    let mut key = Vec::new();
    key_file.read_to_end(&mut key).expect("Could not read key");

    error!("Checking that the private key belongs to the certificate is not implemented");
    error!("Checking that the domain name belongs to the certificate is not implemented");

    let _cert = CertificateDer::from(cert);
    let _key = PrivateKeyDer::from_pem(rustls::pki_types::pem::SectionKind::EcPrivateKey, key);
}


pub(super) fn run(common: Common, cli: Cert)
{
    let verbose = common.verbose;

    let config = Config::read(common.config).expect("Could not parse config file");

    verbose_info!(verbose, "found config: {config:?}");

    assert!(!cli.id.is_empty(), "At least one id required");

    for id in cli.id
    {
        let origin = config.parties.get(&id).expect("Could not find id in config.parties");

        let cert_filename = config.cert_filename(id);

        let key_filename = config.cert_key_filename(id);

        let certificate_file = File::open(&cert_filename);
        let key_file = File::open(&key_filename);

        let cert = match (certificate_file, key_file, cli.force)
        {
            (Err(_), Err(_), false) => create_certificate_files(&cert_filename, &key_filename, &origin.name, verbose),
            (certificate_file, key_file, true) =>
            {
                if cli.force
                {
                    verbose_warn!(verbose && certificate_file.is_ok(), "overwriting certificate file: {cert_filename:?}");
                    verbose_warn!(verbose && key_file.is_ok(), "overwriting key file: {key_filename:?}");
                }
                create_certificate_files(&cert_filename, &key_filename, &origin.name, verbose)
            },
            (Ok(certificate_file), Ok(key_file), false) =>
            {
                check_certificate_files(certificate_file, key_file, &origin.name);
                return;
            },
            (Ok(_), Err(_), false) => panic!("Certificate file ({cert_filename:?}) exists but key file ({key_filename:?}) does not exist. Try to restore the key or create a new pair (--force)."),
            (Err(_), Ok(_), false) => panic!("Certificate file ({cert_filename:?}) does not exist but key file ({key_filename:?}) exist. Try to restore the certificate or create a new pair (--force)."),
        };

        if cli.and_signing_key
        {
            // TODO: also check if files already exist

            let verification_key_filename = config.verification_key_filename(id);
            let signing_key_filename = config.signing_key_filename(id);
            create_signing_files(cert, &verification_key_filename, &signing_key_filename, verbose);
        }
    }
}
