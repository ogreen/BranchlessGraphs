#======================================================================

source ("rvplot2-inc.R")

fit.over.all.graphs <- function (Vars, Df.fit, Df.predict
                               , const.term=FALSE, nonneg=TRUE)
{
  response.var <- if (Vars$has.cycles) "Cycles" else "Time"
  response.true <- sprintf ("%s.true", response.var)
  Data.predicted <- NULL
  Predictions <- NULL

  Fits <- list ()
  Models <- list ()
  Predictors.all <- list ()
  for (arch in unique (Df.fit[["Architecture"]])) {
    for (alg in unique (Df.fit[["Algorithm"]])) {
      for (code in unique (Df.fit[["Implementation"]])) {
        cat (sprintf ("==> %s for %s on %s ...\n", code, alg, arch))
    
        # Choose subset of data to fit
        Data.fit <- subset (Df.fit
                            , Architecture == arch &
                              Algorithm == alg &
                              Implementation == code)

        # Determine predictors
        vars.key <- get.file.suffix (arch, alg, code)
        vars.file <- sprintf ("figs2/explore-corr-vars--%s.txt", vars.key)
        cat (sprintf ("    Loading predictors from: %s ...\n", vars.file))
        if (!file.exists (vars.file)) {
          stop (sprintf ("\n*** Missing model file: '%s' ***\n", vars.file))
        }
        Predictors <- as.vector (unlist (read.table (vars.file)))

        # Fit!
        Fit <- lm.by.colnames (Data.fit, response.var, Predictors
                               , const.term=const.term, nonneg=nonneg)

#        cat (sprintf ("\n=== Fitted model for: %s code for %s on %s ===\n", code, alg, arch))
#        print (summary (Fit))

        # Use fitted model to predict totals
        Data.predict <- subset (Df.predict, Architecture == arch &
                                            Algorithm == alg &
                                            Implementation == code)
        Prediction <- predict.df.lm (Fit, Data.predict, response.var)
        Prediction[, response.true] <- Data.predict[, response.var]
        Prediction <- cbind (Data.predict[, Vars$Index], Prediction)
      
#        cat (sprintf ("\n=== Sample predictions ===\n"))
#        print (head (Prediction))

        Predictions <- rbind.fill (Predictions, Prediction)
        Data.predicted <- rbind.fill (Data.predicted, Data.predict)

        mod.key <- vars.key
        Predictors.all[[mod.key]] <- Predictors
        Models[[mod.key]] <- Fit
      } # for each code
    } # for each alg
  } # for each arch

  Fits$Predictions <- Predictions
  Fits$response.var <- response.var
  Fits$response.true <- response.true
  Fits$Predictors.all <- Predictors.all
  Fits$Models <- Models
  class (Fits) <- c ("cpi.lm", class (Fits))
  return (Fits)
}

fit.one.per.graph <- function (Vars, Df.fit, Df.predict
                               , const.term=FALSE, nonneg=TRUE)
{
  response.var <- if (Vars$has.cycles) "Cycles" else "Time"
  response.true <- sprintf ("%s.true", response.var)
  Data.predicted <- NULL
  Predictions <- NULL

  Fits <- list ()
  Models <- list ()
  Predictors.all <- list ()
  for (arch in unique (Df.fit[["Architecture"]])) {
    for (alg in unique (Df.fit[["Algorithm"]])) {
      for (code in unique (Df.fit[["Implementation"]])) {
        # Determine predictors
        vars.key <- get.file.suffix (arch, alg, code)
        vars.file <- sprintf ("figs2/explore-corr-vars--%s.txt", vars.key)
        cat (sprintf ("    Loading predictors from: %s ...\n", vars.file))
        if (!file.exists (vars.file)) {
          stop (sprintf ("\n*** Missing model file: '%s' ***\n", vars.file))
        }
        Predictors <- as.vector (unlist (read.table (vars.file)))

        # Fit a model per graph
        for (graph in unique (Df.fit[["Graph"]])) {
#          cat (sprintf ("==> %s for %s on %s with input %s ...\n", code, alg, arch, graph))
    
          # Choose subset of training data to fit
          Data.fit <- subset (Df.fit
                              , Architecture == arch &
                                Algorithm == alg &
                                Implementation == code &
                                Graph == graph)

          # Choose subset of testing data to predict
          Data.predict <- subset (Df.predict, Architecture == arch &
                                              Algorithm == alg &
                                              Implementation == code &
                                              Graph == graph)
          
          # Don't even try if there aren't enough samples
          if (nrow (Data.fit) < 2) {
            warning (sprintf ("Insufficient samples (n=%d) to fit: %s code for %s on %s with input %s\n"
                              , nrow (Data.fit)
                              , code, alg, arch, graph))
            Fit <- list ()
            Prediction <- Data.predict[response.var]
            Prediction[, response.true] <- Data.predict[, response.var]
            Prediction[, Predictors] <- 0
            Prediction <- cbind (Data.predict[, Vars$Index], Prediction)
          } else {
            # Fit!
            Fit <- lm.by.colnames (Data.fit, response.var, Predictors
                                   , const.term=const.term, nonneg=nonneg)

#            cat (sprintf ("\n=== Fitted model for: %s code for %s on %s with input %s ===\n", code, alg, arch, graph))
#            print (summary (Fit))

            Prediction <- predict.df.lm (Fit, Data.predict, response.var)
            Prediction[, response.true] <- Data.predict[, response.var]
            Prediction <- cbind (Data.predict[, Vars$Index], Prediction)
          }
      
#          cat (sprintf ("\n=== Sample predictions ===\n"))
#          print (head (Prediction))

          Predictions <- rbind.fill (Predictions, Prediction)

          mod.key <- sprintf ("%s--%s", vars.key, graph)
          Predictors.all[[mod.key]] <- Predictors
          Models[[mod.key]] <- Fit
        } # for each graph
      } # for each code
    } # for each alg
  } # for each arch

  Fits$Predictions <- Predictions
  Fits$response.var <- response.var
  Fits$response.true <- response.true
  Fits$Predictors.all <- Predictors.all
  Fits$Models <- Models
  class (Fits) <- c ("cpi.lm", class (Fits))
  return (Fits)
}

#======================================================================

decode.cpi.lm.model.key <- function (mod.key) {
  tags <- unlist (strsplit (mod.key, split="--"))
  return (c ("arch"=as.character (unlist (ARCHS.ALL.MAP[tags[1]]))
             , "alg"=as.character (unlist (ALGS.ALL.MAP[tags[2]]))
             , "code"=as.character (unlist (CODES.ALL.MAP[tags[3]]))
             , "graph"=tags[4]))
}

# Same as above, but returns a data frame
decode.cpi.lm.model.key.df <- function (mod.key) {
  meta.key <- decode.cpi.lm.model.key (mod.key)
  Meta.df <- data.frame (Architecture=as.character (meta.key["arch"])
                         , Algorithm=as.character (meta.key["alg"])
                         , Implementation=as.character (meta.key["code"]))
  if (!is.na (meta.key["graph"])) {
    Meta.df <- cbind (Meta.df, data.frame (Graph=as.character (meta.key["graph"])))
  }
  return (Meta.df)
}

get.cpi.lm.coefs <- function (Fits) {
  stopifnot ("cpi.lm" %in% class (Fits))

  has.const.term <- ("(Intercept)" %in% colnames (Fits$Predictions))
  
  Predictors.all <- Fits$Predictors.all
  Models.all <- Fits$Models
  Model.summaries <- NULL
  for (mod.key in names (Models.all)) {
    model <- Models.all[[mod.key]]
    if (all (class (model) == "list")) { next }
    
    Meta.df <- decode.cpi.lm.model.key.df (mod.key)
    
    preds <- Predictors.all[[mod.key]]
    if (has.const.term) { preds <- c ("(Intercept)", preds) }
    if ("nnlm" %in% class (model)) {
      coefs <- model$model$x
    } else {
      coefs <- as.vector (model[["coefficients"]])
    }
    Coefs.df <- as.data.frame (t (as.matrix (coefs)))
    colnames (Coefs.df) <- preds

    Diag.df <- data.frame (Mu=model$mu.obs, R.sq=model$res2)

    Model.summary <- cbind (Meta.df, Coefs.df, Diag.df)
    
    Model.summaries <- rbind.fill (Model.summaries, Model.summary
                                   , missing.val=NA)
  }
  return (Model.summaries)
}

get.cpi.lm.residuals <- function (Fits) {
  stopifnot ("cpi.lm" %in% class (Fits))

  has.const.term <- ("(Intercept)" %in% colnames (Fits$Predictions))
  
  Predictors.all <- Fits$Predictors.all
  Models.all <- Fits$Models
  Residuals <- NULL
  for (mod.key in names (Models.all)) {
    model <- Models.all[[mod.key]]
    if (all (class (model) == "list")) { next }

    Meta.mod <- decode.cpi.lm.model.key.df (mod.key)
    R.mod <- data.frame (Y.obs=model$y.obs, Y.pred=model$y.pred
                         , Mu=model$mu.obs, R.sq=model$res2)
    Residuals <- rbind (Residuals, cbind (Meta.mod, R.mod))
  }
  return (Residuals)
}

summary.cpi.lm <- function (Fits) {
  stopifnot ("cpi.lm" %in% class (Fits))
  print (get.cpi.lm.coefs (Fits))
}

#======================================================================
# eof
